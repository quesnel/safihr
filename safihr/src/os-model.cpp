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

#include "global.hpp"
#include <vle/devs/Dynamics.hpp>
#include <vle/devs/DynamicsDbg.hpp>
#include <vle/utils/Trace.hpp>
#include <algorithm>
#include <deque>
#include <stdexcept>
#include <vector>

namespace safihr {

/**
 * Operating System (OS) receives order from the farmer model. Order are
 * sow or harvest. OS stores the order into a list. Messages are sent to
 * (i) farmer when (according to a duration) jobs are finished and to (ii)
 * land unit to perturb the crop model.
 */
class OS : public vle::devs::Dynamics
{
    /**
     * A struct to store when a job finished.
     */
    struct record
    {
        vle::devs::Time end;            // A constant date when message
                                        // must be send.
        vle::devs::Time eta;            // A remaining duration before
                                        // sending message.
        std::string port;               // From which land unit.
        std::string activity;           // Activity name from Agent.
        std::string order;              // Type of order.

        bool operator<(const record &other) const
        {
            return eta < other.eta;
        }
    };

    /**
     * A struct to store message (type: sow or harvest) to land unit (id:
     * port).
     */
    struct message_to_p
    {
        enum MessageType { SOW, HARVEST } ;

        std::string port;
        MessageType msg;
        double duration;
    };

    std::vector <message_to_p> m_message_to_p; // Message to land unit are
                                               // immediatly send.

    std::deque <record> m_queue;        // Sorted deque using C++
                                        // make_heap.
    vle::devs::Time m_time;

    void queue_add(const vle::devs::Time &time,
                   const vle::devs::Time &end,
                   const std::string &activity,
                   const std::string &port,
                   const std::string &order)
    {
        m_queue.push_back(record());

        m_queue.back().end = end;
        m_queue.back().eta = std::max(end - time, 0.0);
        m_queue.back().activity = activity;
        m_queue.back().port = port;
        m_queue.back().order = order;

        std::push_heap(m_queue.begin(), m_queue.end());
    }

    void update_queue(const vle::devs::Time& time)
    {
        for (size_t i = 0, e = m_queue.size(); i != e; ++i)
            m_queue[i].eta = std::max(m_queue[i].end - time, 0.0);

        // Not necessary since m_queue is already sorted with eta?
        std::make_heap(m_queue.begin(), m_queue.end());
    }

public:
    OS(const vle::devs::DynamicsInit &init, const vle::devs::InitEventList &evts)
        : vle::devs::Dynamics(init, evts)
    {}

    virtual ~OS()
    {}

    virtual vle::devs::Time init(const vle::devs::Time &time)
    {
        m_time = time;

        return timeAdvance();
    }

    virtual vle::devs::Time timeAdvance() const
    {
        TraceModel(vle::fmt("1: %1% 2: %2%") %
                   m_message_to_p.size() %
                   m_queue.size());

        if (not m_message_to_p.empty())
            return 0.0;

        if (m_queue.empty())
            return vle::devs::infinity;

        TraceModel(vle::fmt("OS need to be wake up in %1%") % m_queue.front().eta);

        return m_queue.front().eta;
    }

    virtual void internalTransition(const vle::devs::Time &time)
    {
        if (not m_message_to_p.empty())
            m_message_to_p.clear();

        while (not m_queue.empty() && is_almost_equal(m_queue.front().eta, 0.0)) {
            std::pop_heap(m_queue.begin(), m_queue.end());
            m_queue.pop_back();
        }

        update_queue(time);
    }

    virtual void externalTransition(const vle::devs::ExternalEventList& evts,
                                    const vle::devs::Time &time)
    {
        update_queue(time);

        vle::devs::ExternalEventList::const_iterator it, et;
        for (it = evts.begin(), et = evts.end(); it != et; ++it) {
            if ((*it)->onPort("in")) {
                std::string activity = (*it)->getAttributes().getString("activity");
                std::string port = (*it)->getAttributes().getString("p");
                std::string order = (*it)->getAttributes().getString("order");
                double duration = (*it)->getAttributes().getDouble("duration");

                // TODO activities are all done.
                queue_add(time, duration + time, activity, port, "done");

                m_message_to_p.push_back(message_to_p());
                m_message_to_p.back().port = (*it)->getAttributes().getString("p");
                m_message_to_p.back().duration = duration;

                if (order == "sow")
                    m_message_to_p.back().msg = message_to_p::SOW;
            }
        }

        m_time = time;
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
        vle::devs::ExternalEvent *evt = 0;

        if (not m_message_to_p.empty()) {
            std::vector <message_to_p>::const_iterator it, et;

            for (it = m_message_to_p.begin(), et = m_message_to_p.end(); it != et; ++it) {
                evt = new vle::devs::ExternalEvent(it->port);
                evt->putAttribute("order", new vle::value::Integer(it->msg == message_to_p::SOW));
                evt->putAttribute("duration", new vle::value::Double(it->duration));
                output.push_back(evt);
            }
        }

        if (not m_queue.empty())
            TraceModel(vle::fmt("%1% / %2% / %3% / %4%") % m_queue.front().end % time %
                       (is_almost_equal(m_queue.front().eta, 0.0)) %
                       (time - m_queue.front().end));

        if (not m_queue.empty() and is_almost_equal(m_queue.front().eta, 0.0)) {
            std::deque <record> copy(m_queue);

            do {
                evt = new vle::devs::ExternalEvent("out");
                evt->putAttribute("p", new vle::value::String(copy.front().port));
                evt->putAttribute("activity", new vle::value::String(copy.front().activity));
                evt->putAttribute("order", new vle::value::String(copy.front().order));
                output.push_back(evt);

                std::pop_heap(copy.begin(), copy.end());
                copy.pop_back();
            } while (not copy.empty() and is_almost_equal(copy.front().eta, 0.0));
        }
    }

    virtual vle::value::Value * observation(
        const vle::devs::ObservationEvent &event) const
    {
        return vle::devs::Dynamics::observation(event);
    }
};

}

DECLARE_DYNAMICS_DBG(safihr::OS)
