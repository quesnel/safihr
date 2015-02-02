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

#ifndef SAFIHR_STRATEGIC_HPP
#define SAFIHR_STRATEGIC_HPP

#include "global.hpp"
#include <vector>
#include <map>
#include <istream>
#include <ostream>

namespace safihr {

struct CropRotation
{
    CropRotation()
        : current(0u)
    {}

    typedef std::vector <std::string> container_type;
    typedef container_type::value_type value_type;
    typedef container_type::const_iterator const_iterator;
    typedef container_type::iterator iterator;

    // Returns current crop of this rotation
    const std::string& current_crop() const
    {
        return crop_rotation.at(current);
    }

    // Advance current and return new crop.
    const std::string& next_crop()
    {
        return crop_rotation.at(++current);
    }

    container_type crop_rotation;
    size_t current;
};

struct Plots
{
    typedef std::vector <CropRotation> container_type;
    typedef container_type::value_type value_type;
    typedef container_type::const_iterator const_iterator;
    typedef container_type::iterator iterator;

    CropRotation& get(size_t plot) { return plots.at(plot); }
    const CropRotation& get(size_t plot) const { return plots.at(plot); }

    bool empty() const { return plots.empty(); }
    size_t size() const { return plots.size() ;}
    iterator begin() { return plots.begin(); }
    iterator end() { return plots.end(); }
    const_iterator begin() const { return plots.begin(); }
    const_iterator end() const { return plots.end(); }

    container_type plots;
};

std::istream& operator>>(std::istream &is, CropRotation &rotation);
std::ostream& operator<<(std::ostream &os, const CropRotation &rotation);
std::istream& operator>>(std::istream &is, Plots &plots);
std::ostream& operator<<(std::ostream &os, const Plots &plots);

bool operator==(const CropRotation& lhs, const CropRotation& rhs);

}

#endif
