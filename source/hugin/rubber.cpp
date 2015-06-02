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

namespace http = web::http;

namespace navitia { namespace hugin {

void Rubber::create_index(const std::string& index_name) {
    client.request(http::methods::PUT, index_name);
}

void BulkRubber::finish() {
    std::stringstream json_values;
    for (const auto& json: values) {
        json_values << json << "\n";
    }

    std::cout << "query: " << json_values << std::endl;

    rubber.client.request(http::methods::POST, "_bulk", json_values.str());
    values.clear();
}
void BulkRubber::add(const web::json::value& val) {
    values.push_back(val);
}

}}