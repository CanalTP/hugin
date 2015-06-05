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

#include "rubber.h"
#include <cpprest/http_client.h>
#include <cpprest/json.h>
#include <cpprest/filestream.h>
#include <iosfwd>
#include <utils/exception.h>
#include "conf.h"
#include "types.h"


namespace http = web::http;
namespace js = web::json;

namespace navitia { namespace hugin {

std::string UpdateAction::format() const {
    std::stringstream res;
    web::json::value action;
    web::json::value nested_action;
    nested_action["_id"] = js::value::string(id);
    nested_action["_type"] = js::value::string(type);

    action["index"] = js::value(nested_action);

    res << action << "\n" << value;

    return res.str();
}

web::json::value to_geojson(const mpolygon_type& multi_polygon) {
    web::json::value res;

    res["type"] = js::value::string("multipolygon");

    auto multipoly_json = js::value::array();
    size_t cpt_poly(0);
    for (const auto& poly: multi_polygon) {
        auto poly_json = js::value::array();
        size_t  cpt(0);
        for (const auto& coord: poly.outer()) {
            //geojson points are an array with [lon, lat]
            auto point = js::value::array(2);
            point[0] = js::value(coord.get<0>());
            point[1] = js::value(coord.get<1>());

            poly_json[cpt++] = point;
        }
        multipoly_json[cpt_poly++] = poly_json;
    }
    res["coordinates"] = multipoly_json;
    return res;
}

/**
 * Create an elastic search index and configure it with the json setting file
 *
 * the configurations files are store in hugin/fixtures/json/
 */
void Rubber::create_index(const std::string& index_name, const std::string& json_settings) {
    using concurrency::streams::file_stream;
    using concurrency::streams::basic_istream;
    const auto settings_path = std::string(navitia::config::fixtures_dir) + "/json/" + json_settings;

    std::cout << "settings: " << settings_path << std::endl;

    file_stream<unsigned char>::open_istream(settings_path).then([=](pplx::task<basic_istream<unsigned char>> previousTask) {
        try {
            auto fileStream = previousTask.get();

            // Make HTTP request with the file stream as the body.
            client.request(http::methods::PUT, index_name, fileStream).then([&](http::http_response r) {
                if (r.status_code() != 200) {
                    LOG4CPLUS_WARN(logger, "index creation failed with error code: " << r.status_code()
                                           << " reason: " << r.reason_phrase()
                                           << " | " << r.to_string()); //Note: the to_string is expensive, but since we stop on this throw for the moment...
                    throw navitia::exception("index creation failed");
                }
            }).wait();
        } catch (const std::system_error& e) {
            LOG4CPLUS_WARN(logger, "problem with index settings file: " << e.what());
            throw navitia::exception("index settings file error");
        }
    }).wait();

}

pplx::task<web::http::http_response> Rubber::request(
    const web::http::method& method,
    const std::string& path_query,
    const std::string& body_data) {
    std::stringstream real_path;
    real_path << es_index;
    if (es_type) { real_path << "/" << *es_type; }
    real_path << path_query;

    LOG4CPLUS_WARN(logger, real_path.str());

    return client.request(method, real_path.str(), body_data);
}

pplx::task<web::http::http_response> Rubber::checked_request(
        const web::http::method& method,
        const std::string& path_query,
        const std::string& body_data) {

    return request(method, path_query, body_data).then([&] (http::http_response response) {
        if (response.status_code() != 200) {
            LOG4CPLUS_WARN(logger, "bulk insert failed with error code: " << response.status_code()
            << " reason: " << response.reason_phrase()
            << " full: " << response.body());
            throw navitia::exception("builk insert failed");
        }
        return response;
    });
}


void BulkRubber::finish() {
    if (values.empty()) { return; }

    std::stringstream json_values;
    for (const auto& json: values) {
        json_values << json.format() << "\n";
    }
    LOG4CPLUS_DEBUG(logger, "query: " << json_values.str());

    rubber.checked_request(http::methods::POST, "/_bulk", json_values.str()).wait();

    //we clear the value for future finish
    values.clear();
}
void BulkRubber::add(const UpdateAction& val) {
    values.push_back(val);
}

}}