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
#include <exception>
#include "global.hpp"

namespace safihr {

class Crop : public vle::devs::Dynamics
{
    enum CropPhase { WAIT, SOWN, HARVESTABLE, HARVESTED };

    vle::devs::Time  m_begin;
    vle::devs::Time  m_end;
    vle::devs::Time  m_duration;
    vle::devs::Time  m_remaining;
    CropPhase        m_phase;

public:
    Crop(const vle::devs::DynamicsInit &init, const vle::devs::InitEventList &evts)
        : vle::devs::Dynamics(init, evts)
    {
        m_duration = evts.getDouble("duration");
    }

    virtual ~Crop()
    {}

    virtual vle::devs::Time init(const vle::devs::Time &time)
    {
        (void)time;

        m_phase = WAIT;

        return timeAdvance();
    }

    virtual vle::devs::Time timeAdvance() const
    {
        switch (m_phase) {
        case WAIT:
            return vle::devs::infinity;
        case SOWN:
            return m_remaining;
        case HARVESTABLE:
            return vle::devs::infinity;
        case HARVESTED:
            return 0.0;
        }

        throw std::logic_error("crop ta");
    }

    virtual void internalTransition(const vle::devs::Time &time)
    {
        (void)time;

        switch (m_phase) {
        case WAIT:
            throw std::logic_error("crop wait != infinity");
        case SOWN:
            m_phase = HARVESTABLE;
            break;
        case HARVESTABLE:
            throw std::logic_error("crop harvestable != infinity");
        case HARVESTED:
            m_phase = WAIT;
        }
    }

    virtual void externalTransition(const vle::devs::ExternalEventList& evts,
                                    const vle::devs::Time &time)
    {
        vle::devs::ExternalEventList::const_iterator it = evts.begin();
        vle::devs::ExternalEventList::const_iterator et = evts.end();

        for (; it != et; ++it) {
            switch (m_phase) {
            case WAIT:
                if ((*it)->onPort("sow")) {
                    m_begin = time;
                    m_end = time + m_duration;
                    m_remaining = m_duration;
                    m_phase = SOWN;
                }
                break;
            case SOWN:
                m_remaining = m_end - time;
                if (m_remaining < 0.0)
                    throw std::logic_error("crop sown and must be havestable");
                break;
            case HARVESTABLE:
                if ((*it)->onPort("harvest"))
                    m_phase = HARVESTED;
                break;
            case HARVESTED:
                break;
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
        (void)time;

        switch (m_phase) {
        case SOWN:
            //if (is_almost_equal <vle::devs::Time>(m_remaining, 0.0))
            output.push_back(new vle::devs::ExternalEvent("harvestable"));
            break;
        case WAIT:
        case HARVESTABLE:
        case HARVESTED:
            break;
        }
    }

    virtual vle::value::Value * observation(
        const vle::devs::ObservationEvent &event) const
    {
        return vle::devs::Dynamics::observation(event);
    }
};

}

DECLARE_DYNAMICS_DBG(safihr::Crop)
