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
#include <cpprest/http_client.h>
#include <log4cplus/logger.h>
#include <boost/optional.hpp>
#include <boost/range/adaptor/indexed.hpp>
#include "types.h"

namespace navitia { namespace hugin {

struct UpdateAction {
    UpdateAction(const std::string& _id, const std::string _type, const std::string& _index, const web::json::value& val):
        id(_id), type(_type), index(_index), value(val) {}

    const std::string id;
    const std::string type;
    const std::string index;
    web::json::value value;

    std::string format() const;
};

template <typename Col>
web::json::value to_json_array(const Col& collection) {
    web::json::value res = web::json::value::array(collection.size());
    using namespace boost::adaptors;
    const auto indexed_col = collection | indexed(0);
    for (auto it = boost::begin(indexed_col); it != boost::end(indexed_col); ++it) {
        res[it.index()] = web::json::value(*it);
    }
    return res;
}

/**
 * build a geojson from a boost multipolygon
 */
web::json::value to_geojson(const mpolygon_type& multi_polygon);


struct Rubber {
    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");
    Rubber(const std::string& es_bdd): client(es_bdd) {}
    Rubber(const Rubber& r) = default;

    web::http::client::http_client client;

    void create_index(const std::string&, const std::string& json_settings = "index_settings.json");
    std::string es_index;
    boost::optional<std::string> es_type;

    //simple wrapper around http_client to ease use
    pplx::task<web::http::http_response> request(
        const web::http::method& method,
        const std::string& path_query,
        const std::string& body_data);

    //wrapper that throw an exception on error
    pplx::task<web::http::http_response> checked_request(
            const web::http::method& method,
            const std::string& path_query,
            const std::string& body_data);

};

struct BulkRubber {
    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");
    Rubber& rubber;
    std::vector<UpdateAction> values;
    BulkRubber(Rubber& r): rubber(r) {}
    ~BulkRubber() {
        //flush the bulk insert
        finish();
    }
    void finish();
    void add(const UpdateAction&);
};

}}


