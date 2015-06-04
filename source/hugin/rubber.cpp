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
#include <iosfwd>
#include <utils/exception.h>

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

void Rubber::create_index(const std::string& index_name) {
    client.request(http::methods::PUT, index_name).then([=](http::http_response r) {
        if (r.status_code() != 200) {
            LOG4CPLUS_WARN(logger, "index creation failed with error code: " << r.status_code()
            << " reason: " << r.reason_phrase()
            << " full: " << r.body());
            throw navitia::exception("index creationfailed");
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

    rubber.checked_request(http::methods::POST, "/_bulk", json_values.str()).get();

    //we clear the value for future finish
    values.clear();
}
void BulkRubber::add(const UpdateAction& val) {
    values.push_back(val);
}

}}