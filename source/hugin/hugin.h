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

#pragma once
#include "utils/logger.h"
#include "rubber.h"
#include "types.h"
#include <boost/algorithm/string.hpp>

namespace bg = boost::geometry;
typedef bg::model::point<float, 2, bg::cs::cartesian> point;
typedef bg::model::polygon<point, false, false> polygon_type; // ccw, open polygon
typedef bg::model::multi_polygon<polygon_type> mpolygon_type;

namespace navitia { namespace hugin {


struct ReadRelationsVisitor {
    OSMCache& cache;
    ReadRelationsVisitor(OSMCache& cache) : cache(cache) {}

    void node_callback(uint64_t , double , double , const CanalTP::Tags& ) {}
    void relation_callback(uint64_t osm_id, const CanalTP::Tags & tags, const CanalTP::References & refs);
    void way_callback(uint64_t , const CanalTP::Tags& , const std::vector<uint64_t>&) {}
};
struct ReadWaysVisitor {
    // Read references and set if a node is used by a way
    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");
    OSMCache& cache;

    ReadWaysVisitor(OSMCache& cache) : cache(cache) {}

    void node_callback(uint64_t , double , double , const CanalTP::Tags& ) {}
    void relation_callback(uint64_t , const CanalTP::Tags& , const CanalTP::References& ) {}
    void way_callback(uint64_t osm_id, const CanalTP::Tags& tags, const std::vector<uint64_t>& nodes);
};


struct ReadNodesVisitor {
    // Read references and set if a node is used by a way
    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");
    OSMCache& cache;

    ReadNodesVisitor(OSMCache& cache) : cache(cache) {}

    void node_callback(uint64_t osm_id, double lon, double lat, const CanalTP::Tags& tag);
    void relation_callback(uint64_t , const CanalTP::Tags& , const CanalTP::References& ) {}
    void way_callback(uint64_t , const CanalTP::Tags& , const std::vector<uint64_t>&) {}
};

}}

