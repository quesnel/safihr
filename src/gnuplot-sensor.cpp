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
#include "3rd-party/gnuplot_i.hpp"

namespace safihr {

class GnuplotSensor : public vle::devs::Dynamics
{
    Gnuplot m_gnuplot;

    typedef std::map <std::string, std::vector <double> > Data;

    Data m_data;
    std::vector <double> m_time;

public:
    GnuplotSensor(const vle::devs::DynamicsInit &init,
                  const vle::devs::InitEventList &evts)
        : vle::devs::Dynamics(init, evts)
    {}

    virtual ~GnuplotSensor()
    {}

    virtual vle::devs::Time timeAdvance() const
    {
        return vle::devs::infinity;
    }

    virtual void externalTransition(const vle::devs::ExternalEventList& evts,
                                    const vle::devs::Time &time)
    {
        m_time.push_back(time);

        for (vle::devs::ExternalEventList::const_iterator it = evts.begin(),
                 et = evts.end(); it != et; ++it) {

            const vle::value::Map& att = (*it)->getAttributes();
            vle::value::Map::const_iterator jt, ft;
            for (jt = att.begin(), ft = att.end(); jt != ft; ++jt) {
                if (jt->second) {
                    if (jt->second->isDouble()) {
                        m_data[jt->first].push_back(
                            jt->second->toDouble().value());
                    } else if (jt->second->isInteger()) {
                        m_data[jt->first].push_back(
                            static_cast <double>(
                                jt->second->toInteger().value()));
                    }
                }
            }
        }

        for (Data::const_iterator it = m_data.begin(),
                 et = m_data.end(); it != et; ++it) {
            m_gnuplot.set_style("lines").
                plot_xy(m_time, it->second, it->first);
        }

        m_gnuplot.set_xautoscale().replot();
        m_gnuplot.showonscreen();
    }
};

}

DECLARE_DYNAMICS_DBG(safihr::GnuplotSensor)
