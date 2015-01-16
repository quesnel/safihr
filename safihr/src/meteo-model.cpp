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
#include <vle/utils/Package.hpp>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <string>
#include <deque>
#include <exception>
#include "global.hpp"

namespace safihr {

struct MeteoData
{
    double rain;
    double etp;
};

std::istream& operator>>(std::istream &is, MeteoData &data)
{
    std::string temp;

    return is >> temp >> data.rain >> data.etp;
}

struct MeteoCompletedata
{
    std::deque <MeteoData> data;
};

std::istream& operator>>(std::istream &is, MeteoCompletedata &data)
{
    while (is) {
        data.data.push_back(MeteoData());

        is >> data.data.back();
        if (is.fail()) {
            data.data.pop_back();         // the pushed MeteoData
            is.setstate(std::ios::eofbit);
            return is;
        }
    }

    return is;
}

class Meteo : public vle::devs::Dynamics
{
    MeteoCompletedata m_data;
    size_t m_it;

public:
    Meteo(const vle::devs::DynamicsInit &init,
          const vle::devs::InitEventList &evts)
        : vle::devs::Dynamics(init, evts)
    {
        vle::utils::Package package("safihr");

        std::ifstream file(
            package.getDataFile(evts.getString("filename")).c_str());

        if (!file)
            throw std::runtime_error("meteo: failed to open data");

        std::string header;
        std::getline(file, header);

        file >> m_data;
    }

    virtual ~Meteo()
    {}


    virtual vle::devs::Time init(const vle::devs::Time &time)
    {
        (void)time;

        m_it = 0;

        return 0.0;
    }

    virtual vle::devs::Time timeAdvance() const
    {
        return 1.0;
    }

    virtual void internalTransition(const vle::devs::Time &time)
    {
        (void)time;

        if (m_it == m_data.data.size())
            m_it = 0;
        else
            ++m_it;
    }

    virtual void output(const vle::devs::Time &time,
                        vle::devs::ExternalEventList &output) const
    {
        (void)time;

        vle::devs::ExternalEvent *ret = new vle::devs::ExternalEvent("out");
        vle::value::Map &msg = ret->attributes();

        msg.addDouble("rain", m_data.data[m_it].rain);
        msg.addDouble("etp", m_data.data[m_it].etp);

        output.push_back(ret);
    }

    virtual vle::value::Value * observation(
        const vle::devs::ObservationEvent &event) const
    {
        if (event.onPort("rain"))
            return new vle::value::Double(m_data.data[m_it].rain);

        if (event.onPort("etp"))
            return new vle::value::Double(m_data.data[m_it].etp);

        return vle::devs::Dynamics::observation(event);
    }
};

}

DECLARE_DYNAMICS_DBG(safihr::Meteo)
