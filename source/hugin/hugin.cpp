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

#include "hugin.h"
#include <queue>

#include <iostream>
#include <boost/program_options.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "utils/functions.h"
#include "utils/init.h"

#include "conf.h"
#include "persistor.h"

namespace po = boost::program_options;
namespace pt = boost::posix_time;

namespace navitia { namespace hugin {

/*
 * Read relations
 * Stores admins relations and initializes nodes and ways associated to it.
 * Stores relations for associatedStreet, also initializes nodes and ways.
 */
void ReadRelationsVisitor::relation_callback(uint64_t osm_id, const CanalTP::Tags &tags, const CanalTP::References &refs) {
    auto logger = log4cplus::Logger::getInstance("log");
    const auto boundary = tags.find("boundary");
    if (boundary == tags.end() || (boundary->second != "administrative" && boundary->second != "multipolygon")) {
        return;
    }
    const auto tmp_admin_level = tags.find("admin_level");
    if(tmp_admin_level != tags.end() && (tmp_admin_level->second == "8" || tmp_admin_level->second == "9")) {
        for (const CanalTP::Reference& ref : refs) {
            switch(ref.member_type) {
            case OSMPBF::Relation_MemberType::Relation_MemberType_WAY:
                if (ref.role == "outer" || ref.role == "" || ref.role == "exclave") {
                    cache.ways.insert(std::make_pair(ref.member_id, OSMWay()));
                }
                break;
            case OSMPBF::Relation_MemberType::Relation_MemberType_NODE:
                if (ref.role == "admin_centre" || ref.role == "admin_center") {
                    cache.nodes.insert(std::make_pair(ref.member_id, OSMNode()));
                }
                break;
            case OSMPBF::Relation_MemberType::Relation_MemberType_RELATION:
                continue;
            }
        }
        const std::string insee = find_or_default("ref:INSEE", tags);
        const std::string name = find_or_default("name", tags);
        const std::string postal_code = find_or_default("addr:postcode", tags);
        cache.relations.insert(std::make_pair(osm_id, OSMRelation(refs, insee, postal_code, name,
                boost::lexical_cast<uint32_t>(tmp_admin_level->second))));
    }
}

/*
 * We stores ways they are streets.
 * We store ids of needed nodes
 */
void ReadWaysVisitor::way_callback(uint64_t osm_id, const CanalTP::Tags &, const std::vector<uint64_t>& nodes_refs) {
    auto it_way = cache.ways.find(osm_id);
    if (it_way == cache.ways.end()) {
        return;
    }
    for (auto osm_id : nodes_refs) {
        auto v = cache.nodes.insert(std::make_pair(osm_id, OSMNode()));
        it_way->second.add_node(v.first);
    }
}

/*
 * We fill needed nodes with their coordinates
 */
void ReadNodesVisitor::node_callback(uint64_t osm_id, double lon, double lat,
        const CanalTP::Tags& tags) {
    auto node_it = cache.nodes.find(osm_id);
    if (node_it != cache.nodes.end()) {
        node_it->second.set_coord(lon, lat);
        node_it->second.postal_code = find_or_default("addr:postcode", tags);
    }
}


}}

int main(int argc, char** argv) {
    navitia::init_app();
    auto logger = log4cplus::Logger::getInstance("log");
    pt::ptime start;
    std::string input, es_conf;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("version,v", "Show version")
        ("help,h", "Show this message")
        ("input,i", po::value<std::string>(&input)->required(), "Input OSM File")
        ("elastic-search,e", po::value<std::string>(&es_conf)->required(),
         "elastic search location http://localhost:9200");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

    if (vm.count("version")) {
        std::cout << argv[0] << " " << navitia::config::project_version << " "
                  << navitia::config::build_type << std::endl;
        return 0;
    }

    if (vm.count("help")) {
        std::cout << "This program import all cities of an OSM file into a database" << std::endl;
        std::cout << desc << std::endl;
        return 1;
    }

    start = pt::microsec_clock::local_time();
    po::notify(vm);

    navitia::hugin::OSMCache cache;
    navitia::hugin::ReadRelationsVisitor relations_visitor(cache);
    CanalTP::read_osm_pbf(input, relations_visitor);
    navitia::hugin::ReadWaysVisitor ways_visitor(cache);
    CanalTP::read_osm_pbf(input, ways_visitor);
    navitia::hugin::ReadNodesVisitor node_visitor(cache);
    CanalTP::read_osm_pbf(input, node_visitor);
    cache.build_relations_geometries();
    cache.build_postal_codes();

    navitia::hugin::MimirPersistor persistor(cache, es_conf);

    persistor.persist_admins();


    persistor.finish();

    return 0;
}
