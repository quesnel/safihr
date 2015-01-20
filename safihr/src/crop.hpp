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
    std::string id;                     // e.g. F for flax
    std::string name;                   // "flax"
    std::string begin;
    int duration;

    double get_begin(double time) const;
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
