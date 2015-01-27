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

#ifndef SAFIHR_CROP_HPP
#define SAFIHR_CROP_HPP

#include <istream>
#include <ostream>
#include <string>
#include <vector>

namespace safihr {

struct Crop
{
    /// id represents the identifier of the current crops. For example @e
    /// F for @e flax.
    std::string id;

    /// the @e name of the crop. For example @e flax.
    std::string name;

    /// the begin date (@e month-day) when crop can change state from @e
    /// grows to the state @e harvestable.
    unsigned int month;
    unsigned int day;

    /// the duration represents limits of the window to change state from
    /// @e grows to the state @e harvestable.
    int duration;

    /// If @e true, crop starts in beginning of the year. If @e is_summer
    /// is @e false, crop starts during the year and in the same year of
    /// the previous crop.
    bool is_summer;

    double get_begin(unsigned int year) const;
};

struct Crops
{
    typedef std::vector <Crop> container_value;
    typedef container_value::iterator iterator;
    typedef container_value::const_iterator const_iterator;
    typedef container_value::size_type size_type;

    const Crop& get(const std::string& id) const;

    container_value crops;
};

std::istream& operator>>(std::istream &is, Crop &crop);
std::istream& operator>>(std::istream &is, Crops &crops);
std::ostream& operator<<(std::ostream &os, const Crop &crop);
std::ostream& operator<<(std::ostream &os, const Crops &crops);

}

#endif
