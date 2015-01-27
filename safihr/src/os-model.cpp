/*
 * Copyright (C) 2014 INRA
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <vle/devs/Dynamics.hpp>
#include <vle/devs/DynamicsDbg.hpp>
#include <vle/utils/Trace.hpp>
#include <algorithm>
#include <stdexcept>
#include <vector>
#include "global.hpp"

namespace safihr {

struct message_to_plot
{
    struct message
    {
        message(const std::string& plot_, const std::string& crop_,
                const std::string& order_)
            : plot(plot_), crop(crop_), order(order_)
        {}

        std::string plot;
        std::string crop;
        std::string order;
    };

    typedef std::vector <message> container_type;
    typedef container_type::iterator iterator;
    typedef container_type::const_iterator const_iterator;

    void push_back(const std::string& plot, const std::string& crop,
                   const std::string& order)
    {
        assert(order == "sow" or order == "harvest");
        assert(plot.size() > 1 and plot[0] == 'p');

        container.push_back(message(plot, crop, order));
    }

    void send(vle::devs::ExternalEventList &output) const
    {
        for (const_iterator it = container.begin(); it != container.end(); ++it) {
            vle::devs::ExternalEvent *evt = new vle::devs::ExternalEvent(it->plot);
            evt->putAttribute("order", new vle::value::String(it->order));
            evt->putAttribute("crop", new vle::value::String(it->crop));

            output.push_back(evt);
        }
    }

    void clear() { return container.clear(); }
    bool empty() const { return container.empty(); }

    container_type container;
};

std::ostream& operator<<(std::ostream& os, const message_to_plot& msgs)
{
    os << "message to plot ";
    for (size_t i = 0, e = msgs.container.size(); i != e; ++i)
        os << '(' << msgs.container[i].plot
           << ',' << msgs.container[i].crop
           << ',' << msgs.container[i].order << ')';

    return os;
}

struct message_to_farmer
{
    struct message
    {
        message(const vle::devs::Time& end_, const vle::devs::Time& eta_,
                const std::string& port_, const std::string& activity_,
                const std::string& order_)
            : end(end_), eta(eta_), port(port_), activity(activity_)
            , order(order_)
        {}

        vle::devs::Time end;            // A constant date when message must be send.
        vle::devs::Time eta;            // A remaining duration before sending message.
        std::string port;               // From which land unit.
        std::string activity;           // Activity name from Agent.
        std::string order;              // Type of order.
    };

    struct message_compare
    {
        bool operator()(const message& a, const message& b) const
        {
            return a.eta >= b.eta;
        }
    };

    void push_back(const vle::devs::Time &time, const vle::devs::Time &end,
                   const std::string &activity, const std::string &port,
                   const std::string &order)
    {
        container.push_back(message(end, std::max(end - time, 0.0), port, activity, order));
        std::push_heap(container.begin(), container.end(), message_compare());
    }

    void pop(const vle::devs::Time& time)
    {
        while (not container.empty() and container.front().end == time) {
            std::pop_heap(container.begin(), container.end(), message_compare());
            container.pop_back();
        }
    }

    void update(const vle::devs::Time& time)
    {
        for (size_t i = 0, e = container.size(); i != e; ++i)
            container[i].eta = std::max(container[i].end - time, 0.0);

        std::make_heap(container.begin(), container.end(), message_compare());
    }

    void send(const vle::devs::Time& time, vle::devs::ExternalEventList &output) const
    {
        container_type copy(container);

        while (not copy.empty() and copy.front().end == time) {
            vle::devs::ExternalEvent *evt = new vle::devs::ExternalEvent("out");
            evt->putAttribute("p", new vle::value::String(copy.front().port));
            evt->putAttribute("activity", new vle::value::String(copy.front().activity));
            evt->putAttribute("order", new vle::value::String("done"));
            output.push_back(evt);

            std::pop_heap(copy.begin(), copy.end(), message_compare());
            copy.pop_back();
        }
    }

    vle::devs::Time top() const { return container.front().eta; }
    bool empty() const { return container.empty(); }

    typedef std::vector <message> container_type;
    typedef container_type::iterator iterator;
    typedef container_type::const_iterator const_iterator;

    container_type container;
};

std::ostream& operator<<(std::ostream& os, const message_to_farmer& msgs)
{
    os << "message to farmer ";

    message_to_farmer::container_type c(msgs.container);
    while (not c.empty()) {
        os << '(' << c.front().activity << ',' << c.front().port << ','
           << c.front().order << ',' << c.front().eta << ','
           << c.front().end << ')';

        std::pop_heap(c.begin(), c.end(), message_to_farmer::message_compare());
        c.pop_back();
    }

    return os;
}

/**
 * Operating System (OS) receives order from the farmer model. Order are
 * sow or harvest. OS stores the order into a list. Messages are sent to
 * (i) farmer when (according to a duration) jobs are finished and to (ii)
 * land unit to perturb the crop model.
 */
class OS : public vle::devs::Dynamics
{
    message_to_plot m_message_to_plot; // Message be send immediatly to
                                       // plot.
    message_to_farmer m_message_to_farmer; // Message to be send to farmer
                                           // after the duration of work.
public:
    OS(const vle::devs::DynamicsInit &init, const vle::devs::InitEventList &evts)
        : vle::devs::Dynamics(init, evts)
    {}

    virtual ~OS()
    {}

    virtual vle::devs::Time timeAdvance() const
    {
        if (not m_message_to_plot.empty())
            return 0.0;

        if (not m_message_to_farmer.empty())
            return m_message_to_farmer.top();

        return vle::devs::infinity;
    }

    virtual void internalTransition(const vle::devs::Time &time)
    {
        m_message_to_plot.clear();
        m_message_to_farmer.pop(time);

        m_message_to_farmer.update(time);
    }

    virtual void externalTransition(const vle::devs::ExternalEventList& evts,
                                    const vle::devs::Time &time)
    {
        m_message_to_farmer.update(time);

        vle::devs::ExternalEventList::const_iterator it, et;
        for (it = evts.begin(), et = evts.end(); it != et; ++it) {
            if ((*it)->onPort("in")) {
                std::string activity = (*it)->getAttributes().getString("activity");
                std::string port = (*it)->getAttributes().getString("p");
                std::string crop = (*it)->getAttributes().getString("crop");
                std::string order = (*it)->getAttributes().getString("order");
                double duration = (*it)->getAttributes().getDouble("duration");

                if (order == "sow" or order == "harvest")
                    m_message_to_plot.push_back(port, crop, order);

                m_message_to_farmer.push_back(time, duration + time, activity, port, order);
            }
        }
    }

    virtual void confluentTransition(const vle::devs::ExternalEventList& evts,
                                     const vle::devs::Time &time)
    {
        internalTransition(time);
        externalTransition(evts, time);
    }

    virtual void output(const vle::devs::Time &time,
                        vle::devs::ExternalEventList &output) const
    {
        m_message_to_plot.send(output);
        m_message_to_farmer.send(time, output);
    }

    virtual vle::value::Value * observation(
        const vle::devs::ObservationEvent &event) const
    {
        return vle::devs::Dynamics::observation(event);
    }
};

}

DECLARE_DYNAMICS_DBG(safihr::OS)
