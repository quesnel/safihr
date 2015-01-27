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

#ifndef SAFIHR_GLOBAL_HPP
#define SAFIHR_GLOBAL_HPP

#include <vle/utils/i18n.hpp>
#include <vle/utils/Exception.hpp>
#include <boost/algorithm/string/iter_find.hpp>
#include <boost/algorithm/string/finder.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/cast.hpp>
#include <exception>
#include <limits>
#include <map>
#include <string>

namespace safihr {

static const char *model_names[] = { "Farmer", "OperatingSystem", "Meteo" };

enum ModelType
{
    FarmerName = 0,
    OperatingSystemName = 1,
    MeteoName = 2
};

inline std::string farmer_model_name()
{
    return model_names[static_cast <int>(FarmerName)];
}

inline std::string operatingsystem_model_name()
{
    return model_names[static_cast <int>(OperatingSystemName)];
}

inline std::string meteo_model_name()
{
    return model_names[static_cast <int>(MeteoName)];
}

inline std::string landunit_model_name(int i)
{
    char buffer[std::numeric_limits<int>::digits10 + 2];

    int written = std::snprintf(buffer, sizeof(buffer), "p%d", i);

    return std::string(&buffer[0], written);
}

inline void split_activity_name(const std::string& activity,
                                std::string *operation,
                                std::string *crop,
                                std::string *plot,
                                int *index)
{
    std::vector <boost::iterator_range <std::string::const_iterator> > v;
    v.reserve(5u);

    boost::algorithm::iter_split(v, activity,
                                 boost::algorithm::token_finder(
                                     boost::algorithm::is_any_of("_")));

    switch (v.size()) {
    case 3u:
        if (operation)
            operation->assign(v[0].begin(), v[0].end());
        if (crop)
            crop->assign(v[1].begin(), v[1].end());
        if (plot)
            plot->assign(v[2].begin(), v[2].end());
        break;
    case 4u:
        if (operation)
            operation->assign(v[0].begin(), v[0].end());
        if (crop)
            crop->assign(v[2].begin(), v[2].end());
        if (plot)
            plot->assign(v[3].begin(), v[3].end());
        if (index) {
            try {
                *index = boost::lexical_cast <int>(
                    std::string(v[1].begin(), v[1].end()));
            } catch (...) {
                throw vle::utils::ModellingError(
                    vle::fmt("Bad activity name: %1%") % activity);
            }
        }
        break;
    default:
        throw vle::utils::ModellingError(
            vle::fmt("Bad activity name: %1%") % activity);
    }
}


inline double stod(const std::string &str)
{
    try {
        return boost::lexical_cast <double>(str);
    } catch (...) {
        throw std::invalid_argument("stod");
    }
}

inline int stoi(const std::string &str)
{
    try {
        long int ret = boost::lexical_cast <long int>(str);
        return boost::numeric_cast <int>(ret);
    } catch (...) {
        throw std::invalid_argument("stoi");
    }
}

template <typename T>
bool is_almost_equal(const T a, const T b)
{
//     const T scale = (std::abs(a) + std::abs(b)) / T(2.0);
//     return std::abs(a - b) <= (scale * std::numeric_limits<T>::epsilon());

    return std::abs(a - b) <= 0.1;
}

}

#endif
