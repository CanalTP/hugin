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

#include "types.h"
#include "utils/functions.h"
#include <boost/geometry.hpp>
#include <boost/range/algorithm/find_if.hpp>

namespace navitia { namespace hugin {

/*
 *  Builds geometries of relations
 */
void OSMCache::build_relations_geometries() {
    auto logger = log4cplus::Logger::getInstance("log");

    size_t cpt_empty_relations(0);
    for (auto& relation: relations) {
        relation.second.build_geometry(*this);

        if (relation.second.polygon.empty()) {
            cpt_empty_relations++;
            continue;
        }
        boost::geometry::model::box<point> box;
        boost::geometry::envelope(relation.second.polygon, box);
        Rect r(box.min_corner().get<0>(), box.min_corner().get<1>(),
               box.max_corner().get<0>(), box.max_corner().get<1>());
        admin_tree.Insert(r.min, r.max, &relation.second);
    }
    if (cpt_empty_relations) {
        LOG4CPLUS_DEBUG(logger, cpt_empty_relations << " have been ignored because of empty polygon");
    }
}

void OSMCache::build_zip_codes(){
    for (auto relation: relations) {
        if(relation.second.level != 9){
            continue;
        }
        auto rel = this->match_coord_admin(relation.second.centre.get<0>(), relation.second.centre.get<1>(), 8);
        if(rel){
            rel->zip_codes.insert(relation.second.zip_codes.begin(), relation.second.zip_codes.end());
        }
    }
}

OSMRelation* OSMCache::match_coord_admin(const double lon, const double lat, uint32_t level) {
    Rect search_rect(lon, lat);
    const auto p = point(lon, lat);
    typedef std::pair<uint32_t, std::vector<OSMRelation*>*> level_relations;

    std::vector<OSMRelation*> result;
    auto callback = [](OSMRelation* rel, void* c) -> bool {
        level_relations* context;
        context = reinterpret_cast<level_relations*>(c);
        if(rel->level == context->first){
            context->second->push_back(rel);
        }
        return true;
    };
    level_relations context = std::make_pair(level, &result);
    admin_tree.Search(search_rect.min, search_rect.max, callback, &context);
    for(auto rel : result) {
        if (boost::geometry::within(p, rel->polygon)){
            return rel;
        }
    }
    return nullptr;
}

std::string OSMNode::to_geographic_point() const{
    std::stringstream geog;
    geog << std::setprecision(10)<<"POINT("<< coord_to_string() <<")";
    return geog.str();
}

OSMRelation::OSMRelation(const std::vector<CanalTP::Reference>& refs, const std::string& insee,
                         const std::string zip_code, const std::string& name, const uint32_t level) :
        references(refs), insee(insee), name(name), level(level) {
    this->add_zip_code(zip_code);
}

void OSMRelation::add_zip_code(const std::string& zip_code){
    if (zip_code.empty()) {
        return;
    } else if(zip_code.find(";", 0) == std::string::npos) {
        this->zip_codes.insert(zip_code);
    } else {
        boost::split(this->zip_codes, zip_code, boost::is_any_of(";"));
    }
}

/*
 * We build the polygon of the admin
 * Ways are supposed to be order, but they're not always.
 * Also we may have to reverse way before adding them into the polygon
 */
void OSMRelation::build_polygon(OSMCache& cache, std::set<u_int64_t> explored_ids) {
    auto is_outer_way = [](const CanalTP::Reference& r) {
        return r.member_type == OSMPBF::Relation_MemberType::Relation_MemberType_WAY
               && in(r.role, {"outer", "enclave", ""});
    };
    auto pickable_way = [&](const CanalTP::Reference& r) {
        return is_outer_way(r) && explored_ids.count(r.member_id) == 0;
    };
    // We need to explore every node because a boundary can be made in several parts
    while(explored_ids.size() != this->references.size()) {
        // We pickup one way
        auto ref = boost::find_if(references, pickable_way);
        if (ref == references.end()) {
            break;
        }
        auto it_first_way = cache.ways.find(ref->member_id);
        if (it_first_way == cache.ways.end() || it_first_way->second.nodes.empty()) {
            break;
        }
        auto first_node = it_first_way->second.nodes.front()->first;
        auto next_node = it_first_way->second.nodes.back()->first;
        explored_ids.insert(ref->member_id);
        polygon_type tmp_polygon;
        size_t debug_cpt(0); //DEBUG! to limit the number of points in geometry
        for (auto node : it_first_way->second.nodes) {
            if (!node->second.is_defined()) {
                continue;
            }
            const auto p = point(float(node->second.lon()), float(node->second.lat()));
            tmp_polygon.outer().push_back(p);

            if (debug_cpt > 5) { break; }//DEBUG!!
        }

        // We try to find a closed ring
        while (first_node != next_node) {
            // We look for a way that begin or end by the last node
            ref = boost::find_if(references,
                                 [&](CanalTP::Reference& r) {
                                     if (!pickable_way(r)) {
                                         return false;
                                     }
                                     auto it = cache.ways.find(r.member_id);
                                     return it != cache.ways.end() &&
                                            !it->second.nodes.empty() &&
                                            (it->second.nodes.front()->first == next_node ||
                                             it->second.nodes.back()->first == next_node );
                                 });
            if (ref == references.end()) {
                break;
            }
            explored_ids.insert(ref->member_id);
            auto next_way = cache.ways[ref->member_id];
            if (next_way.nodes.front()->first != next_node) {
                boost::reverse(next_way.nodes);
            }
            for (auto node : next_way.nodes) {
                if (!node->second.is_defined()) {
                    continue;
                }
                const auto p = point(float(node->second.lon()), float(node->second.lat()));
                tmp_polygon.outer().push_back(p);
                if (debug_cpt > 5) { break; }//DEBUG!!
            }
            next_node = next_way.nodes.back()->first;
        }
        if (tmp_polygon.outer().size() < 2 || ref == references.end()) {
            break;
        }
        const auto front = tmp_polygon.outer().front();
        const auto back = tmp_polygon.outer().back();
        // This should not happen, but does some time
        if (front.get<0>() != back.get<0>() || front.get<1>() != back.get<1>()) {
            tmp_polygon.outer().push_back(tmp_polygon.outer().front());
        }
        polygon.push_back(tmp_polygon);
    }
    if ((centre.get<0>() == 0.0 || centre.get<1>() == 0.0) && !polygon.empty()) {
        bg::centroid(polygon, centre);
    }
}

void OSMRelation::build_geometry(OSMCache& cache) {
    for (CanalTP::Reference ref: references) {
        if (ref.member_type == OSMPBF::Relation_MemberType::Relation_MemberType_NODE) {
            auto node_it = cache.nodes.find(ref.member_id);
            if (node_it == cache.nodes.end()) {
                continue;
            }
            if (!node_it->second.is_defined()) {
                continue;
            }
            if (in(ref.role, {"admin_centre", "admin_center"})) {
                set_centre(float(node_it->second.lon()), float(node_it->second.lat()));
                this->add_zip_code(node_it->second.zip_code);
                break;
            }
        }
    }
    build_polygon(cache);
}

}}