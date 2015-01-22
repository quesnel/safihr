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

#include "strategic.hpp"
#include <stdexcept>

namespace safihr {

std::istream& operator>>(std::istream &is, CropRotation &rotation)
{
    std::string tmp;
    is >> tmp;                          /* Avoid ID string comment */

    while (is.good() && is.peek() != '\n') {
        rotation.years.push_back(-1);
        is >> rotation.years.back();
    }

    while (is.good()) {
        int id;
        is >> id;

        if (is.fail() or is.eof()) {
            is.clear(is.eofbit);
            break;
        }

        auto ret = rotation.crops.insert(
            std::make_pair(id, std::vector <std::string>()));
        if (!ret.second)
            throw std::runtime_error("same land unit define multiple times");

        ret.first->second.reserve(rotation.years.size());
        while (is.good() && is.peek() != '\n') {
            is >> tmp;
            ret.first->second.emplace_back(tmp);
        }

        // Here, we check the validity of data. Each crops must have the
        // same number of crop than the size of the vector years.
        if (ret.first->second.size() != rotation.years.size())
            throw std::runtime_error("crops rotation have not the same number"
                                     "of crop");
    }

    return is;
}

std::ostream& operator<<(std::ostream &os, const CropRotation &rotation)
{
    os << "ID";

    for (auto year : rotation.years)
        os << "\t" << year;

    for (const auto& crops : rotation.crops) {
        os << '\n' << crops.first;

        for (const auto& crop : crops.second)
            os << "\t" << crop;
    }

    return os;
}

bool operator==(const CropRotation& lhs, const CropRotation& rhs)
{
    return lhs.years == rhs.years && lhs.crops == rhs.crops;
}

}
