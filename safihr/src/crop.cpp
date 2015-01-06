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
#include <fstream>
#include <boost/assign.hpp>

namespace safihr {

std::istream& operator>>(std::istream &is, Crop &crop)
{
    if (is) {
        std::string tmp;

        is >> tmp >> crop.surf_min >> crop.surf_max >> crop.dr >> crop.repete_min
            >> crop.repete_max;

        if (!is)
            goto failure;

        crop.crop_id = string_to_crop(tmp);

        for (int i = 0; i < 7; ++i) {
            is >> tmp;

            if (!is)
                goto failure;

            crop.prec[i] = (tmp == "oui");
        }

        if (is)
            return is;
    }

failure:
    is.setstate(std::ios::failbit);

    return is;
}

std::istream& operator>>(std::istream& is, Crops &crops)
{
    while (is) {
        crops.crops.push_back(Crop());
        is >> crops.crops.back();
    }

    return is;
}

}
