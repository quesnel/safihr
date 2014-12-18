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
#include <algorithm>
#include <deque>
#include <stdexcept>
#include <vector>

namespace safihr {

class OS : public vle::devs::Dynamics
{
    struct record
    {
        vle::devs::Time end;
        vle::devs::Time eta;
        std::string port;
        std::string order;

        bool operator<(const record &other) const
        {
            return eta < other.eta;
        }
    };

    struct message_to_p
    {
        enum MessageType { SOW, HARVEST } ;

        std::string port;
        MessageType msg;
    };

    std::vector <message_to_p> m_message_to_p;
    std::deque <record> m_queue;
    vle::devs::Time m_time;

    void queue_add(const vle::devs::Time &time,
                   const vle::devs::Time &end,
                   const std::string &port,
                   const std::string &order)
    {
        m_queue.push_back(record());

        m_queue.back().end = end;
        m_queue.back().eta = std::max(end - time, 0.0);
        m_queue.back().port = port;
        m_queue.back().order = order;

        std::push_heap(m_queue.begin(), m_queue.end());
    }

    void update_queue(const vle::devs::Time& time)
    {
        for (size_t i = 0, e = m_queue.size(); i != e; ++i)
            m_queue[i].eta = std::max(m_queue[i].end - time, 0.0);

        // Not necessary since m_queue is already sorted with eta?
        // std::make_heap(m_queue.begin(), m_queue.end());
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
        if (not m_message_to_p.empty())
            return 0.0;

        if (m_queue.empty())
            return vle::devs::infinity;

        return m_queue.front().eta;
    }

    virtual void internalTransition(const vle::devs::Time &time)
    {
        if (not m_message_to_p.empty())
            m_message_to_p.clear();

        while (is_almost_equal(m_queue.front().end, time)) {
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
                std::string port = (*it)->getAttributes().getString("p");
                std::string order = (*it)->getAttributes().getString("order");
                double duration = (*it)->getAttributes().getDouble("duration");
                queue_add(time, duration + time, port, order);

                m_message_to_p.push_back(message_to_p());
                m_message_to_p.back().port = (*it)->getAttributes().getString("p");

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
                output.push_back(evt);
            }
        }

        if (not m_queue.empty() && is_almost_equal(m_queue.front().end, time)) {
            std::deque <record> copy(m_queue);

            while (is_almost_equal(copy.front().end, time)) {
                evt = new vle::devs::ExternalEvent("out");
                evt->putAttribute("p", new vle::value::String(copy.front().port));
                evt->putAttribute("order", new vle::value::String(copy.front().order));
                output.push_back(evt);

                std::pop_heap(copy.begin(), copy.end());
                copy.pop_back();
            }
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
