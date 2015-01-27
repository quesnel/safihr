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

#include "crop.hpp"
#include "global.hpp"
#include <boost/assign.hpp>
#include <algorithm>
#include <iterator>
#include <fstream>
#include <vle/utils/i18n.hpp>
#include <vle/utils/DateTime.hpp>
#include <vle/utils/Exception.hpp>

namespace safihr {

double Crop::get_begin(unsigned int year) const
{
    std::string date = (vle::fmt("%1%-%2%-%3%") % year % month % day).str();

    return vle::utils::DateTime::toJulianDayNumber(date);
}

struct CropFind
{
    bool operator()(const Crop& a, const std::string& b) const
    {
        return a.id < b;
    }

    bool operator()(const std::string& a, const Crop& b) const
    {
        return a < b.id;
    }
};

const Crop& Crops::get(const std::string& id) const
{
    const_iterator it = std::lower_bound(crops.begin(), crops.end(),
                                         id,
                                         CropFind());

    if (it == crops.end() or it->id != id)
        throw vle::utils::ModellingError(
            vle::fmt("fails to find crop id %1%") % id);

    return *it;
}

std::istream& operator>>(std::istream &is, Crop &crop)
{
    std::string type;

    is >> crop.id >> crop.name >> type >> crop.month >> crop.day
       >> crop.duration;

    crop.is_summer = (type == "summer");

    return is;
}

struct CropCompare
{
    bool operator()(const Crop& a, const Crop& b) const
    {
        return a.id < b.id;
    }
};

std::istream& operator>>(std::istream& is, Crops &crops)
{
    std::string line;
    std::getline(is, line);

    crops.crops.clear();
    crops.crops.reserve(8u);

    while (is.good()) {
        crops.crops.push_back(Crop());
        is >> crops.crops.back();

        if (is.fail() or is.eof()) {
            crops.crops.pop_back();
            is.clear(is.eofbit);
        }
    }

    std::sort(crops.crops.begin(), crops.crops.end(),
              CropCompare());

    return is;
}

std::ostream& operator<<(std::ostream& os, const Crop& crop)
{
    return os << crop.name << crop.name << '\t'
              << ((crop.is_summer) ? "summer" : "winter") << '\t'
              << crop.month << '\t' << crop.day << '\t' << crop.duration;
}

std::ostream& operator<<(std::ostream& os, const Crops& crops)
{
    os << "Id\tName\tType\tDate (y-m-d)\tRandom duration (day)\n";

    std::copy(crops.crops.begin(),
              crops.crops.end(),
              std::ostream_iterator <Crop>(os, "\n"));

    return os;
}

}
