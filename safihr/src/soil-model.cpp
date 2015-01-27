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

/*
 * Variable d'entrée :
 * P(i) en mm : pluie du jour i
 * ETP(i) en mm : ETP du jour i
 *
 * Variable sortie
 * RU(i) en mm = réserve utile du jour i
 *
 * Initialisation au 01/01 :
 * RU(01/01) = 40
 *
 * Fonction d'actualisation de RU(i) :
 * RU(i)=max(0;(min(40;(RU(i-1)+P(i)-((Ru(i-1)/40)*ETP(i))))))
 *
 */


class Soil : public vle::devs::Dynamics
{
    enum SoilPhase { WAIT, SEND };

    double    m_p;
    double    m_etp;
    double    m_ru;
    SoilPhase m_phase;
    int       m_received;

public:
    Soil(const vle::devs::DynamicsInit &init, const vle::devs::InitEventList &evts)
        : vle::devs::Dynamics(init, evts)
    {}

    virtual ~Soil()
    {
    }

    virtual vle::devs::Time init(const vle::devs::Time &time)
    {
        (void)time;

        m_p = -HUGE_VAL;
        m_etp = -HUGE_VAL;

        m_ru = 40.0;
        m_phase = WAIT;
        m_received = 2;

        return timeAdvance();
    }

    virtual vle::devs::Time timeAdvance() const
    {
        switch (m_phase) {
        case WAIT:
            return vle::devs::infinity;
        case SEND:
            return 0.0;
        }

        throw std::logic_error("soil-model: ta");
    }

    virtual void internalTransition(const vle::devs::Time &time)
    {
        (void)time;

        switch (m_phase) {
        case WAIT:
            throw std::logic_error("crop wait != infinity");
        case SEND:
            m_phase = WAIT;
            m_received = 2;
            break;
        }
    }

    virtual void externalTransition(const vle::devs::ExternalEventList &evts,
                                    const vle::devs::Time &time)
    {
        (void)time;

        vle::devs::ExternalEventList::const_iterator it = evts.begin();
        vle::devs::ExternalEventList::const_iterator et = evts.end();
        for (; it != et; ++it) {
            if ((*it)->onPort("in")) {
                if ((*it)->getAttributes().exist("rain")) {
                    --m_received;
                    m_p = (*it)->getDoubleAttributeValue("rain");
                }

                if ((*it)->getAttributes().exist("etp")) {
                    m_etp = (*it)->getDoubleAttributeValue("etp");
                    --m_received;
                }
            }
        }

        if (m_received == 0) {
            m_ru = std::max(0.0,
                            std::min(40.0,
                                     m_ru + m_p - ((m_ru / 40.0) * m_etp)));

            m_phase = SEND;
        }
    }

    virtual void confluentTransition(const vle::devs::ExternalEventList &evts,
                                     const vle::devs::Time &time)
    {
        internalTransition(time);
        externalTransition(evts, time);
    }

    virtual void output(const vle::devs::Time &time,
                        vle::devs::ExternalEventList &output) const
    {
        (void)time;

        if (m_phase == SEND) {
            vle::devs::ExternalEvent *evt = new vle::devs::ExternalEvent("out");
            evt->putAttribute("ru", new vle::value::Double(m_ru));
            output.push_back(evt);
        }
    }

    virtual vle::value::Value * observation(
        const vle::devs::ObservationEvent &event) const
    {
        if (event.onPort("ru"))
            return new vle::value::Double(m_ru);

        return vle::devs::Dynamics::observation(event);
    }
};

}

DECLARE_DYNAMICS_DBG(safihr::Soil)
