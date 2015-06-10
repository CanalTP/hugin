/* Copyright © 2001-2015, Canal TP and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!

LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

Stay tuned using
twitter @navitia
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE hugin_test
#include <boost/test/unit_test.hpp>
#include "hugin/types.h"
#include "hugin/persistor.h"
#include "utils/logger.h"

struct logger_initializer {
    logger_initializer() { init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initializer)

using web::json::value;


const auto epsilon = 10e-6;

BOOST_AUTO_TEST_CASE(geojson_creator) {
    // test the creation of a geojson from a boost::geometry
    mpolygon_type multi_polygon;
    polygon_type square;
    square.outer().push_back({0., 0.});
    square.outer().push_back({0., 1.});
    square.outer().push_back({1., 1.});
    square.outer().push_back({1., 0.});
    multi_polygon.push_back(square);

    polygon_type triangle;
    triangle.outer().push_back({2., 0.});
    triangle.outer().push_back({4., 0.});
    triangle.outer().push_back({3., 1.});
    multi_polygon.push_back(triangle);

    const auto geojson = navitia::hugin::to_geojson(multi_polygon);

    BOOST_CHECK_EQUAL(geojson.at("type"), value::string("multipolygon"));
    BOOST_REQUIRE(geojson.has_field("coordinates"));

    const auto coordinates = geojson.at("coordinates"); //first coordinates is a one elt wrapper
    BOOST_REQUIRE_EQUAL(coordinates.size(), 1);
    const auto multi_poly = coordinates.at(0); //the real multipoly is here
    BOOST_REQUIRE_EQUAL(multi_poly.size(), 2);

    const auto square_geo = multi_poly.at(0);
    BOOST_REQUIRE_EQUAL(square_geo.size(), 4);
    BOOST_REQUIRE_EQUAL(square_geo.at(0).size(), 2);
    BOOST_REQUIRE_CLOSE(square_geo.at(0).at(0).as_double(), 0., epsilon);
    BOOST_REQUIRE_CLOSE(square_geo.at(0).at(1).as_double(), 0., epsilon);
    BOOST_REQUIRE_EQUAL(square_geo.at(1).size(), 2);
    BOOST_REQUIRE_CLOSE(square_geo.at(1).at(0).as_double(), 0., epsilon);
    BOOST_REQUIRE_CLOSE(square_geo.at(1).at(1).as_double(), 1., epsilon);
    BOOST_REQUIRE_EQUAL(square_geo.at(2).size(), 2);
    BOOST_REQUIRE_CLOSE(square_geo.at(2).at(0).as_double(), 1., epsilon);
    BOOST_REQUIRE_CLOSE(square_geo.at(2).at(1).as_double(), 1., epsilon);
    BOOST_REQUIRE_EQUAL(square_geo.at(3).size(), 2);
    BOOST_REQUIRE_CLOSE(square_geo.at(3).at(0).as_double(), 1., epsilon);
    BOOST_REQUIRE_CLOSE(square_geo.at(3).at(1).as_double(), 0., epsilon);

    const auto triangle_geo = multi_poly.at(1);
    BOOST_REQUIRE_EQUAL(triangle_geo.size(), 3);
    BOOST_REQUIRE_EQUAL(triangle_geo.at(0).size(), 2);
    BOOST_REQUIRE_CLOSE(triangle_geo.at(0).at(0).as_double(), 2., epsilon);
    BOOST_REQUIRE_CLOSE(triangle_geo.at(0).at(1).as_double(), 0., epsilon);
    BOOST_REQUIRE_EQUAL(triangle_geo.at(1).size(), 2);
    BOOST_REQUIRE_CLOSE(triangle_geo.at(1).at(0).as_double(), 4., epsilon);
    BOOST_REQUIRE_CLOSE(triangle_geo.at(1).at(1).as_double(), 0., epsilon);
    BOOST_REQUIRE_EQUAL(triangle_geo.at(2).size(), 2);
    BOOST_REQUIRE_CLOSE(triangle_geo.at(2).at(0).as_double(), 3., epsilon);
    BOOST_REQUIRE_CLOSE(triangle_geo.at(2).at(1).as_double(), 1., epsilon);
}
