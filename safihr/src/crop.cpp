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

namespace safihr {

double Crop::get_begin(long year) const
{
    std::string date = (vle::fmt("%1%-%2%") % year % begin).str();

    return vle::utils::DateTime::toJulianDay(date);
}

std::istream& operator>>(std::istream &is, Crop &crop)
{
    return is >> crop.name >> crop.begin >> crop.duration;
}

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

    return is;
}

std::ostream& operator<<(std::ostream& os, const Crop& crop)
{
    return os << crop.name << crop.name << '\t' << crop.begin
              << '\t' << crop.duration;
}

std::ostream& operator<<(std::ostream& os, const Crops& crops)
{
    os << "CropId\tDate (y-m-d)\tDuration (day)\n";

    std::copy(crops.crops.begin(),
              crops.crops.end(),
              std::ostream_iterator <Crop>(os, "\n"));

    return os;
}

}
