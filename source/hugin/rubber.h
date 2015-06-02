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

namespace navitia { namespace hugin {

struct Rubber {
    Rubber(const std::string& es_bdd): client(es_bdd) {}
    Rubber(const Rubber& r) = default;

    web::http::client::http_client client;
    void bob() {
//        std::cout << "query: " << std::endl;
//        auto res = client.request(web::http::methods::GET, U("mimir"));
//
//        auto real_res = res.get();
//        auto json = real_res.extract_json().get();
//        std::cout << json << std::endl;
//
//        std::cout << json["mimir"] << std::endl;
//        std::cout << json["mimir"]["settings"]["index"] << std::endl;
//        std::cout << json["mimir"]["settings"]["index"]["number_of_replicas"].as_number().to_int64() << std::endl;
    }

    void create_index(const std::string&);

};

struct BulkRubber {
    Rubber& rubber;
    std::vector<web::json::value> values;
    BulkRubber(Rubber& r): rubber(r) {}
    ~BulkRubber() {
        //flush the bulk insert
        finish();
    }
    void finish();
    void add(const web::json::value&);
};

}}

