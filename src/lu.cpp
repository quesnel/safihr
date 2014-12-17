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

#include "lu.hpp"
#include <fstream>

namespace safihr {

std::istream& operator>>(std::istream &is, LandUnit &lu)
{
    return is >> lu.id >> lu.lu_min >> lu.lu_max >> lu.nb_lu >> lu.sau;
}

std::istream& operator>>(std::istream& is, LandUnits &lus)
{
    std::string header;
    std::getline(is, header);           // Avoid the header

    while (is) {
        lus.lus.push_back(LandUnit());

        if (!(is >> lus.lus.back()))    // Try to read, if it fails
            lus.lus.pop_back();         // remove the data.
    }

    return is;
}

std::ostream& operator<<(std::ostream &os, LandUnit &lu)
{
    return os << lu.id << "\t" << lu.lu_min << "\t"
              << lu.lu_max << "\t" << lu.nb_lu << "\t"
              << lu.sau;
}

std::ostream& operator<<(std::ostream &os, LandUnits &lus)
{
    os << "ID\tLU-min\tLU-max\tNb-LU\tSAU";

    for (size_t i = 0, e = lus.lus.size(); i != e; ++i)
        os << '\n' << lus.lus[i];

    os << '\n';

    return os;
}

bool operator==(const LandUnit &lhs, const LandUnit &rhs)
{
    return lhs.id == rhs.id &&
        lhs.lu_min == rhs.lu_min &&
        lhs.lu_max == rhs.lu_max &&
        lhs.nb_lu == rhs.nb_lu &&
        lhs.sau == rhs.sau;
}

bool operator==(const LandUnits &lhs, const LandUnits &rhs)
{
    return lhs.lus == rhs.lus;
}

}
