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

#include "persistor.h"
#include <boost/geometry.hpp>

#include <cpprest/json.h>
#include <log4cplus/logger.h>


namespace js = web::json;

namespace navitia { namespace hugin {

MimirPersistor::MimirPersistor(const OSMCache& cache, const std::string& conf, const std::string& index):
        data(cache), es_index(index), rubber(conf) {
    rubber.es_index = es_index;
}

void MimirPersistor::persist_admins() {

    size_t nb_empty_polygons = 0;
    size_t nb_added_admin = 0;
    BulkRubber bulk(rubber);
    for (const auto& relation: data.relations) {
        if (relation.second.polygon.empty()) {
            ++nb_empty_polygons;
            continue;
        }

        js::value val;
        const auto uri = "admin:osm:" + std::to_string(relation.first);
        val["name"] = js::value::string(relation.second.name);

        val["id"] = js::value::string(uri);
        val["zip_codes"] = to_json_array(relation.second.zip_codes);
        val["insee"] = js::value::string(relation.second.insee);
        val["level"] = relation.second.level;

        val["coord"]["lat"] = js::value::number(relation.second.centre.get<0>());
        val["coord"]["lon"] = js::value::number(relation.second.centre.get<1>());

        const auto shape = to_geojson(relation.second.polygon);
        val["shape"] = shape;
        val["admin_shape"] = shape;
        val["weight"] = 0; // TODO

        UpdateAction action(uri, "admin", es_index, val);
        bulk.add(action);
        nb_added_admin++;
    }
    bulk.finish();
    auto logger = log4cplus::Logger::getInstance("log");
    LOG4CPLUS_INFO(logger, nb_added_admin << " admin added, "
    << nb_empty_polygons << " ignored admins because their polygons were empty");
}

void MimirPersistor::create_index() {
    //TODO we need to be able to know if the settings have changed, because if so, we need to delete and create again the index

    rubber.create_index(es_index);
}

void MimirPersistor::finish() {

}
}}