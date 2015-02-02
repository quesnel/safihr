/*
 * This file is part of VLE, a framework for multi-modeling, simulation
 * and analysis of complex dynamical systems.
 * http://www.vle-project.org
 *
 * Copyright (c) 2003-2014 Gauthier Quesnel <quesnel@users.sourceforge.net>
 * Copyright (c) 2003-2014 ULCO http://www.univ-littoral.fr
 * Copyright (c) 2007-2014 INRA http://www.inra.fr
 *
 * See the AUTHORS or Authors.txt file for copyright owners and
 * contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#define BOOST_TEST_MAIN
#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE package_test
#include <boost/test/unit_test.hpp>
#include <boost/test/auto_unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

#include "lu.hpp"
#include "crop.hpp"
#include "strategic.hpp"
#include <vle/utils/Package.hpp>
#include <vle/utils/Path.hpp>
#include <vle/utils/ModuleManager.hpp>
#include <vle/vle.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

struct F
{
    vle::Init app;

    F() : app() {}
    ~F() {}
};

BOOST_GLOBAL_FIXTURE(F)

BOOST_AUTO_TEST_CASE(test_crop_rotation)
{
    vle::utils::Package pack("safihr");
    std::string filepath(pack.getDataFile("Assolement-test.txt"));
    std::ifstream ifs(filepath.c_str());
    BOOST_REQUIRE(ifs.is_open());

    safihr::CropRotation cr1, cr2;
    ifs >> cr1;
    BOOST_REQUIRE(ifs.eof());

    std::stringstream ss;
    ss << cr1;
    ss >> cr2;
}

BOOST_AUTO_TEST_CASE(test_lus)
{
    vle::utils::Package pack("safihr");
    std::string filepath(pack.getDataFile("Farm.txt"));
    std::ifstream ifs(filepath.c_str());
    BOOST_REQUIRE(ifs.is_open());

    safihr::LandUnits lu1, lu2;
    ifs >> lu1;
    BOOST_REQUIRE(ifs.eof());

    std::cout << lu1 << "\n";

    std::stringstream ss;
    ss << lu1;
    ss >> lu2;

    BOOST_REQUIRE(lu1 == lu2);
}
