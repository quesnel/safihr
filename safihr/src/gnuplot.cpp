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
#include <vector>
#include <map>
#include <vle/utils/DateTime.hpp>
#include <vle/utils/Package.hpp>
#include "global.hpp"
#include "gnuplot.hpp"

namespace safihr {

static void write_gnuplot(std::ostream& os,
                          const std::string& data_filename,
                          vle::devs::Time min,
                          vle::devs::Time max,
                          double width,
                          double height,
                          const std::string& title,
                          const std::string& subtitle)
{
    min -= 1.0;
    max += 1.0;

    os << std::fixed
       << "set terminal svg size " << std::max(1.0, width) * 1000.0
       << "," << std::max(1.0, height) * 400.0
       << " dynamic enhanced fname 'arial'  fsize 10 mousing name \""
       << "Gantt\" butt dashlength 1.0\n"
       << "set output '" << data_filename << ".svg' \n"
       << "set border 3 front lt 'black' linewidth 1.000\n"
       << "set xdata time\n"
       << "set format x \"%b\\n'%y\"\n"
       << "set grid nopolar\n"
       << "set grid xtics nomxtics ytics nomytics noztics nomztics nox2tics "
       << "nomx2tics noy2tics nomy2tics nocbtics nomcbtics\n"
       << "set grid layerdefault   lt 'black' linewidth 0.200,  lt 'black' linewidth 0.200\n"
       << "unset key\n"
       << "set style arrow 1 nohead size 5 fixed lw 5 filled \n"
       // << "set style arrow 1 head back filled linecolor rgb \"#56b4e9\"  "
       // << "linewidth 1.500 size screen  0.020,15.000,90.000  fixed\n"
       << "set style data lines\n"
       << "set mxtics 4.000000\n"
       << "set xtics border in scale 2,0.5 nomirror norotate  autojustify\n"
       << "set xtics " << min << ", " << max << "\n"
       << "set ytics border in scale 1,0.5 nomirror norotate  autojustify\n"
       << "set ytics  norangelimit\n"
       << "set ytics   ()\n"
       << "set title \"{/=15 " << title << "}\\n\\n{/:Bold " << subtitle << "}\" \n"
//       << "set xrange [" << min << ":" << max << "] noreverse nowriteback\n"
       << "set yrange [ -1.00000 : * ] noreverse nowriteback\n"
       << "T(N) = timecolumn(N,timeformat)\n"
       << "timeformat = \"%Y-%m-%d\"\n"
       << "OneMonth = " << min << "\n"
       << "GPFUN_T = \"T(N) = timecolumn(N,timeformat)\"\n"
       << "x = 0.0\n"
       << "plot '" << data_filename << ".dat' using (T(2)) : ($0) : (T(3)-T(2)) :"
       << "(0.0) : yticlabel(1) with vector as 1,"
       << "     '" << data_filename << ".dat' using (T(2)) : ($0) : "
       << "1 with labels right offset -2\n";
}

struct gnuplot_observation
{
    gnuplot_observation(const std::string& task_, double begin_, double end_)
        : task(task_)
        , begin(begin_)
        , end(end_)
    {}

    gnuplot_observation()
    {}

    std::string task;
    double begin;
    double end;
};

std::ostream& operator<<(std::ostream& os, const gnuplot_observation& item)
{
    return os << item.task << '\t'
              << vle::utils::DateTime::toJulianDayNumber(item.begin) << '\t'
              << vle::utils::DateTime::toJulianDayNumber(item.end) << '\n';
}

struct gnuplot_observation_compare
{
    bool operator()(const gnuplot_observation& a, const gnuplot_observation& b) const
    {
        return a.begin < b.begin;
    }
};

struct complete_observation
{
    complete_observation(const std::string& crop_, const std::string& plot_,
                         const std::string& operation_, int year_,
                         double begin_, double end_)
        : crop(crop_), plot(plot_), operation(operation_), year(year_),
          begin(begin_), end(end_)
    {}

    std::string crop, plot, operation;
    int year;
    double begin, end;
};

std::ostream& operator<<(std::ostream& os, const complete_observation& obs)
{
    return os << obs.begin << ';'
              << obs.end << ';'
              << vle::utils::DateTime::toJulianDay(obs.begin) << ';'
              << vle::utils::DateTime::toJulianDay(obs.end) << ';'
              << obs.crop << ';'
              << obs.plot << ';'
              << obs.operation << ';'
              << obs.year << ';';
}

struct complete_observation_compare
{
    bool operator()(const complete_observation& a,
                    const complete_observation& b) const
    {
        return a.begin < b.begin;
    }
};

struct gnuplot_plot_observation
{
    typedef std::map <std::string, std::vector <gnuplot_observation> > container_type;
    typedef container_type::value_type value_type;
    typedef container_type::const_iterator const_iterator;
    typedef container_type::iterator iterator;

    void insert(const std::string& crop, const std::string& plot,
                const std::string& operation, int year, double begin, double end)
    {
        std::pair <iterator, bool> r = lst.insert(
            value_type(
                plot, std::vector <gnuplot_observation>()));

        r.first->second.push_back(
            gnuplot_observation(
                (vle::fmt("%1%-%2%-%3%") % operation % crop % year).str(),
                begin, end));
    }

    void sort()
    {
        for (iterator it = lst.begin(); it != lst.end(); ++it)
            std::sort(it->second.begin(), it->second.end(),
                      gnuplot_observation_compare());
    }

    void write(vle::devs::Time min, vle::devs::Time max) const
    {
        for (const_iterator it = lst.begin(); it != lst.end(); ++it) {
            std::string data = (vle::fmt("%1%.dat") % it->first).str();
            std::string gp = (vle::fmt("%1%.gp") % it->first).str();

            {
                std::ofstream ofs(data.c_str());
                ofs << "#Task\tstart\tend\n";

                for (size_t i = 0, e = it->second.size(); i != e; ++i)
                    ofs << it->second[i];
            }

            {
                std::ofstream ofs(gp.c_str());
                write_gnuplot(ofs, it->first, min, max, 2.0, 1.0,
                              (vle::fmt("Gantt %1%") % it->first).str(),
                              "");
            }
        }
    }

    container_type lst;
};

struct gnuplot_crop_po_observation
{
    typedef std::map <std::string, std::vector <gnuplot_observation> > container_type;
    typedef container_type::value_type value_type;
    typedef container_type::const_iterator const_iterator;
    typedef container_type::iterator iterator;

    void insert(const std::string& crop, const std::string& plot,
                const std::string& operation, int year, double begin, double end)
    {
        std::pair <iterator, bool> r = lst.insert(
            value_type(
                (vle::fmt("%1%-%2%-%3%") % crop % year % plot).str(),
                std::vector <gnuplot_observation>()));

        r.first->second.push_back(
            gnuplot_observation(
                operation, begin, end));
    }

    void sort()
    {
        for (iterator it = lst.begin(); it != lst.end(); ++it)
            std::sort(it->second.begin(), it->second.end(),
                      gnuplot_observation_compare());
    }

    void write(vle::devs::Time min, vle::devs::Time max) const
    {
        for (const_iterator it = lst.begin(); it != lst.end(); ++it) {
            std::string data = (vle::fmt("%1%.dat") % it->first).str();
            std::string gp = (vle::fmt("%1%.gp") % it->first).str();

            {
                std::ofstream ofs(data.c_str());
                ofs << "#Task\tstart\tend\n";

                for (size_t i = 0, e = it->second.size(); i != e; ++i)
                    ofs << it->second[i];
            }

            {
                std::ofstream ofs(gp.c_str());
                write_gnuplot(ofs, it->first, min, max, 1.0, 1.0,
                              (vle::fmt("Gantt %1%") % it->first).str(),
                              "");
            }
        }
    }

    container_type lst;
};

struct gnuplot_all_observation
{
    typedef std::vector <gnuplot_observation> container_type;
    typedef container_type::value_type value_type;
    typedef container_type::const_iterator const_iterator;
    typedef container_type::iterator iterator;

    void insert(const std::string& crop, const std::string& plot,
                const std::string& operation, int year, double begin, double end)
    {
        lst.push_back(
            gnuplot_observation(
                (vle::fmt("%1%-%2%-%3%-%4%") % plot % year % crop % operation).str(),
                begin, end));
    }

    void sort()
    {
        std::sort(lst.begin(), lst.end(), gnuplot_observation_compare());
    }

    void write(vle::devs::Time min, vle::devs::Time max) const
    {
        std::string data = "ITK-all.dat";
        std::string gp = "ITK-all.gp";

        {
            std::ofstream ofs(data.c_str());
            ofs << "#Task\tstart\tend\n";

            for (size_t i = 0, e = lst.size(); i != e; ++i)
                ofs << lst[i];
        }

        {
            std::ofstream ofs(gp.c_str());

            write_gnuplot(ofs, "ITK-all", min, max, 4.0, 10.0,
                          "Gant all plots", "");
        }
    }

    container_type lst;
};

struct csv_complete_observation
{
    typedef std::set <complete_observation, complete_observation_compare> container_type;
    typedef container_type::value_type value_type;
    typedef container_type::const_iterator const_iterator;
    typedef container_type::iterator iterator;

    void insert(const std::string& crop, const std::string& plot,
                const std::string& operation, int year, double begin, double end)
    {
        lst.insert(
            complete_observation(
                crop, plot, operation, year, begin, end));
    }

    void write()
    {
        std::ofstream ofs("complete.csv");

        ofs << "start;end;start date;end date;crop;plot;operation;year\n"
            << std::fixed;

        std::copy(lst.begin(), lst.end(),
                  std::ostream_iterator <complete_observation>(
                      ofs, "\n"));
    }

    container_type lst;
};

struct grantt_observation
{
    grantt_observation(const vle::extension::decision::Activities& activities)
        : min(vle::devs::infinity)
        , max(vle::devs::negativeInfinity)
    {
        vle::extension::decision::Activities::const_iterator it, et;
        std::string operation, crop, plot;
        int index, year;

        for (it = activities.begin(), et = activities.end();  it != et; ++it) {
            split_activity_name(it->first, &operation, &index, &crop, &year, &plot);

            double begin = it->second.startedDate();
            if (vle::devs::isInfinity(begin))
                continue;

            double end = it->second.doneDate();
            if (vle::devs::isInfinity(end)) {
                end = it->second.ffDate();
                if (vle::devs::isInfinity(end))
                    continue;
            }

            min = std::min(min, begin);
            max = std::max(max, end);

            crops.insert(crop, plot, operation, year, begin, end);
            plots.insert(crop, plot, operation, year, begin, end);
            all.insert(crop, plot, operation, year, begin, end);
            csv.insert(crop, plot, operation, year, begin, end);
        }

        crops.sort();
        plots.sort();
        all.sort();
    }

    void write()
    {
        crops.write(min, max);
        plots.write(min, max);
        all.write(min, max);
        csv.write();
    }

    gnuplot_crop_po_observation crops;
    gnuplot_plot_observation plots;
    gnuplot_all_observation all;
    csv_complete_observation csv;
    double min, max;
};


void build_grantt_observation(const vle::extension::decision::Activities& activities)
{
    vle::utils::Package pack("safihr");
    grantt_observation grantt(activities);

    grantt.write();
}

}
