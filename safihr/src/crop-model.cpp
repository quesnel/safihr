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
#include <vle/utils/DateTime.hpp>
#include <vle/utils/Package.hpp>
#include <vle/utils/Exception.hpp>
#include <vle/utils/Rand.hpp>
#include <vle/utils/Trace.hpp>
#include <exception>
#include <fstream>
#include "global.hpp"
#include "crop.hpp"

namespace safihr {

class CropModel : public vle::devs::Dynamics
{
    enum CropPhase { WAIT, SOWN, HARVESTABLE, HARVESTED };

    Crops            m_crop;

    vle::utils::Rand m_rand;
    vle::devs::Time  m_begin;
    vle::devs::Time  m_end;
    vle::devs::Time  m_duration;
    vle::devs::Time  m_remaining;
    int              m_number;
    CropPhase        m_phase;

public:
    CropModel(const vle::devs::DynamicsInit &init,
              const vle::devs::InitEventList &evts)
        : vle::devs::Dynamics(init, evts)
    {
        vle::utils::Package pack("safihr");

        std::ifstream ifs(pack.getDataFile("Crop.txt").c_str());
        if (!ifs.is_open())
            throw vle::utils::ModellingError(
                vle::fmt("crop: fails to open %1%") % "Crop.txt");

        ifs >> m_crop;

        if (ifs.fail())
            throw vle::utils::ModellingError(
                vle::fmt("crop: error while reading file %1%") %
                "Crop.txt");

        m_rand.seed(3);

        // m_rand.seed((unsigned long int)(this));

        // TODO We must use a seed taken from conditions.
        //     m_rand.seed(
        //         static_cast <vle::utils::Rand::result_type>(
        //             evts.getInt("seed")));
    }

    virtual ~CropModel()
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
            return 1.0;
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

    void compute_harvestable_date(const vle::devs::Time &current_time,
                                  const std::string& cropid,
                                  vle::devs::Time* duration,
                                  int* number)
    {
        const Crop& c = m_crop.get(cropid);

        unsigned int year = vle::utils::DateTime::year(current_time);
        unsigned int month = vle::utils::DateTime::month(current_time);
        unsigned int day = vle::utils::DateTime::dayOfMonth(current_time);
        unsigned int to_add = 0;

        if (month > c.month)
            to_add = 1;
        else if (month == c.month && day > c.day)
                to_add = 1;

        *duration = c.get_begin(year + to_add) + m_rand.getInt(1, c.duration) - current_time;
        *number = (cropid != "SB") ? 1 : 3;
    }

    virtual void externalTransition(const vle::devs::ExternalEventList& evts,
                                    const vle::devs::Time &time)
    {
        vle::devs::ExternalEventList::const_iterator it = evts.begin();
        vle::devs::ExternalEventList::const_iterator et = evts.end();

        for (; it != et; ++it) {
            assert((*it)->onPort("in"));

            switch (m_phase) {
            case WAIT:
                assert(((*it)->getAttributes().getString("order")) == "sow");

                compute_harvestable_date(time,
                                         (*it)->getAttributes().getString("crop"),
                                         &m_duration,
                                         &m_number);

                DTraceModel(vle::fmt("crop sown: remaining in %1% days") % m_duration);

                m_begin = time;
                m_end = time + m_duration;
                m_remaining = m_duration;
                m_phase = SOWN;
                break;
            case SOWN:
                DTraceModel(vle::fmt("crop receives order %1% during SOWN stage")
                            %(*it)->getAttributes().getString("crop"));

                m_remaining = m_end - time;
                if (m_remaining < 0.0)
                    throw std::logic_error("crop sown and must be havestable");
                break;
            case HARVESTABLE:
                assert(((*it)->getAttributes().getString("order")) == "harvest");

                DTraceModel(vle::fmt("crop receivre order %1% with number = %2%")
                            %(*it)->getAttributes().getString("crop") % m_number);

                --m_number;
                if (m_number == 0)
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

        if (m_phase == SOWN) {
            vle::devs::ExternalEvent *evt = new vle::devs::ExternalEvent("out");
            evt->putAttribute("harvestable", new vle::value::Boolean(true));
            output.push_back(evt);
        }
    }

    virtual vle::value::Value * observation(
        const vle::devs::ObservationEvent &event) const
    {
        if (event.onPort("phase")) {
            switch (m_phase) {
            case SOWN:
                return new vle::value::String("sown");
            case WAIT:
                return new vle::value::String("-   ");
            case HARVESTABLE:
                return new vle::value::String("harv");
            case HARVESTED:
                return new vle::value::String("end ");
            }
        }

        return vle::devs::Dynamics::observation(event);
    }
};

}

DECLARE_DYNAMICS_DBG(safihr::CropModel)
