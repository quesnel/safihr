/*
 * Copyright (C) 2015 INRA
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

#include <string>
#include <fstream>
#include <iterator>
#include <vle/utils/DateTime.hpp>
#include <vle/utils/Package.hpp>
#include <boost/unordered_map.hpp>
#include "global.hpp"
#include "gnuplot.hpp"

namespace safihr {

struct grantt_observation
{
    struct item
    {
        item(const std::string& task_, double begin_, double end_)
            : task(task_)
              , begin(begin_)
              , end(end_)
        {}

        std::string task;
        double begin;
        double end;
    };

    grantt_observation(const vle::extension::decision::Activities& activities)
        : min(vle::devs::infinity)
    {
        vle::extension::decision::Activities::const_iterator it, et;
        std::string operation, plot;

        for (it = activities.begin(), et = activities.end();  it != et; ++it) {
            split_activity_name(it->first, &operation, 0, &plot, 0);

            double begin = it->second.startedDate();
            if (vle::devs::isNegativeInfinity(begin))
                continue;

            double end = it->second.doneDate();
            if (vle::devs::isNegativeInfinity(end)) {
                end = it->second.ffDate();
                if (vle::devs::isNegativeInfinity(end))
                    continue;
            }

            if (min > begin)
                min = begin;

            lst.insert(
                std::make_pair <std::string, item>(
                    plot, item(it->first, begin, end)));
        }
    }

    typedef boost::unordered_multimap <std::string, item> container_type;
    typedef container_type::const_iterator const_iterator;
    typedef container_type::iterator iterator;

    container_type lst;
    double min;
};

std::ostream& operator<<(std::ostream& os, const grantt_observation::item& item)
{
    return os << item.task << '\t'
              << vle::utils::DateTime::toJulianDayNumber(item.begin) << '\t'
              << vle::utils::DateTime::toJulianDayNumber(item.end) << '\n';
}

std::ostream& operator<<(std::ostream& os, const grantt_observation& grantt)
{
    os << "#Task\tstart\tend\n";

    for (grantt_observation::const_iterator it = grantt.lst.begin(),
         et = grantt.lst.end(); it != et; ++it)
        os << it->second;

    return os;
}

static void write_gnuplot(std::ostream& os, const std::string& plot,
                          vle::devs::Time min, int size)
{
    os <<  "set terminal svg size 600," << size * 400
       << " dynamic enhanced fname 'arial'  fsize 10 mousing name \"gantt_"
        << plot << "\" butt dashlength 1.0\n"
        << "set output 'gantt." << plot << ".svg'\n"
        << "set border 3 front lt 'black' linewidth 1.000\n"
        << "set xdata time\n"
        << "set format x \"%b\\n'%y\"\n"
        << "set grid nopolar\n"
        << "set grid xtics nomxtics ytics nomytics noztics nomztics nox2tics nomx2tics noy2tics nomy2tics nocbtics nomcbtics\n"
        << "set grid layerdefault   lt 'black' linewidth 0.200,  lt 'black' linewidth 0.200\n"
        << "unset key\n"
        << "set style arrow 1 head back filled linecolor rgb \"#56b4e9\"  linewidth 1.500 size screen  0.020,15.000,90.000  fixed\n"
        << "set style data lines\n"
        << "set mxtics 4.000000\n"
        << "set xtics border in scale 2,0.5 nomirror norotate  autojustify\n"
        << "set xtics 2.6784e+06 norangelimit\n"
        << "set ytics border in scale 1,0.5 nomirror norotate  autojustify\n"
        << "set ytics  norangelimit\n"
        << "set ytics   ()\n"
        << "set title \"{/=15 Simple Gantt Chart}\\n\\n{/:Bold Task start and end times in columns 2 and 3}\" \n"
        << "set yrange [ -1.00000 : * ] noreverse nowriteback\n"
        << "T(N) = timecolumn(N,timeformat)\n"
        << "timeformat = \"%Y-%m-%d\"\n"
        << "OneMonth = " << min << "\n"
        << "GPFUN_T = \"T(N) = timecolumn(N,timeformat)\"\n"
        << "x = 0.0\n"
        << "plot 'ITK-" << plot <<  ".dat' using (T(2)) : ($0) : (T(3)-T(2)) : (0.0) : yticlabel(1) with vector as 1,"
        << "     'ITK-" << plot <<  ".dat' using (T(2)) : ($0) : 1 with labels right offset -2\n";
}

void build_grantt_observation(const vle::extension::decision::Activities& activities)
{
    vle::utils::Package pack("safihr");
    grantt_observation grantt(activities);

    {
        std::ofstream ofs("ITK-all.dat");
        assert(ofs.is_open());
        ofs << grantt;
    }

    {
        std::ofstream ofs("ITKs.gp");
        write_gnuplot(ofs, "all", grantt.min, 5);
    }

    grantt_observation::const_iterator it = grantt.lst.begin();
    while (it != grantt.lst.end()) {

        grantt_observation::const_iterator prev = it;

        {
            std::ofstream ofs((vle::fmt("ITK-%1%.dat") % it->first).str().c_str());
            assert(ofs.is_open());
            ofs << "#Task\tstart\tend\n";

            do {
                ofs << it->second;
                prev = it++;
            } while (it != grantt.lst.end() and prev->first == it->first);
        }

        {
            std::ofstream ofs((vle::fmt("ITK-%1%.gp") % prev->first).str().c_str());
            assert(ofs.is_open());

            write_gnuplot(ofs, prev->first, grantt.min, 1);
        }
    }

}

}
