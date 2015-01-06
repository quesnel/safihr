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
#include <exception>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include "global.hpp"
#include "3rd-party/gnuplot_i.hpp"

namespace safihr {

class GnuplotSensor : public vle::devs::Dynamics
{
    typedef std::vector <std::vector <double> > Data;
    typedef Data::size_type Id;
    typedef std::map <std::string, Id> ColumnId;

    size_t column(const std::string &str_id)
    {
        std::pair <ColumnId::iterator, bool> r;

        r = m_id.insert(ColumnId::value_type(str_id, m_data.size()));

        if (r.second)
            m_data.push_back(std::vector <double>());

        return r.first->second;
    }

    void write()
    {
        std::ofstream ofs(m_filename.c_str());
        if (!ofs)
            throw vle::utils::ModellingError(
                vle::fmt("Sensor: fails to open tempory file %1%") % m_filename);

        for (size_t j = 0, f = m_time.size(); j != f; ++j) {
            ofs << m_time[j] << '\t';

            for (size_t i = 0, e = m_data.size(); i != e; ++i)
                ofs << m_data[i][j] << '\t';

            ofs << '\n';
        }
    }

    void show()
    {
        m_gnuplot_list.push_back(boost::shared_ptr <Gnuplot>(new Gnuplot));

        m_gnuplot_list.back()->set_terminal_std("qt persist");
        m_gnuplot_list.back()->set_grid();
        m_gnuplot_list.back()->set_style("lines");
        m_gnuplot_list.back()->set_xlabel("time");
        m_gnuplot_list.back()->set_xautoscale();
        m_gnuplot_list.back()->set_yautoscale();

        for (ColumnId::const_iterator it = m_id.begin(), et = m_id.end();
             it != et; ++it) {
            m_gnuplot_list.back()->set_title(m_modelname);
            m_gnuplot_list.back()->plotfile_xy(m_filename, 1, it->second + 2, it->first);
        }

        m_gnuplot_list.back()->showonscreen();
        m_gnuplot_list.back()->replot();
    }

    std::vector <boost::shared_ptr <Gnuplot> > m_gnuplot_list;
    ColumnId m_id;
    Data m_data;
    std::vector <double> m_time;
    std::string m_filename;
    std::string m_modelname;
    int m_print_limit;
    int m_print;

public:
    GnuplotSensor(const vle::devs::DynamicsInit &init,
                  const vle::devs::InitEventList &evts)
        : vle::devs::Dynamics(init, evts)
        , m_print_limit(365)
        , m_print(m_print_limit)
    {
        vle::utils::Package package("safihr");
        m_filename = package.getOutputFile(
            (vle::fmt("sensor-%1%") % this).str());

        if (evts.exist("sensor-update")) {
            m_print_limit = evts.getInt("sensor-update");
            m_print = m_print_limit;
        }
    }

    virtual ~GnuplotSensor()
    {}

    virtual vle::devs::Time timeAdvance() const
    {
        return vle::devs::infinity;
    }

    virtual vle::devs::Time init(const vle::devs::Time &)
    {
        m_modelname = getModel().getCompleteName();

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
                    size_t id = column(jt->first);

                    if (jt->second->isDouble()) {
                        m_data[id].push_back(jt->second->toDouble().value());
                    } else if (jt->second->isInteger()) {
                        m_data[id].push_back(static_cast <double>(
                                                 jt->second->toInteger().value()));
                    }
                }
            }
        }

        if (m_print_limit > 0) {
            m_print--;

            if (m_print <= 0) {
                m_print = m_print_limit;
                write();
                show();
            }
        }
    }

    virtual void finish()
    {
        if (m_print_limit <= 0) {
            write();
            show();
        }
    }
};

}

DECLARE_DYNAMICS_DBG(safihr::GnuplotSensor)
