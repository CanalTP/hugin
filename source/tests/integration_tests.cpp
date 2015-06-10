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
#include "fixtures.h"

#include "hugin/types.h"
#include "hugin/persistor.h"

#include <boost/geometry.hpp>

BOOST_GLOBAL_FIXTURE(es_global_fixture)

using web::http::client::http_client;
using web::http::http_response;
using web::http::methods;
namespace js = web::json;
namespace ba = boost::algorithm;

//used as http query handler helper in the tests
struct es_integration_tests_fixture {
    std::string elastic_search_url = "http://localhost:9201/";

    http_client client;
    es_integration_tests_fixture() : client(elastic_search_url) {}
};

BOOST_FIXTURE_TEST_SUITE(simple_elastic_import, es_integration_tests_fixture)

BOOST_AUTO_TEST_CASE(create_index)
{
    navitia::hugin::Rubber rubber(elastic_search_url);

    rubber.create_index("toto");

    client.request(methods::GET, "/toto").then([] (http_response r) {
        BOOST_REQUIRE_EQUAL(r.status_code(), 200);
    }).wait();
}

BOOST_AUTO_TEST_CASE(add_one_admin)
{
    navitia::hugin::OSMCache cache;
    navitia::hugin::MimirPersistor persistor(cache, elastic_search_url, "tata");

    navitia::hugin::OSMRelation admin({}, "bob", "postal12", "bob's bob", 42);
    admin.centre = point(1.0, 2.0);
    admin.zip_codes.insert("postal42");
    polygon_type poly;
    poly.outer().push_back({2.0, 1.3});
    poly.outer().push_back({4.1, 3.0});
    poly.outer().push_back({5.3, 2.6});
    poly.outer().push_back({2.0, 1.3});
    admin.polygon.push_back(poly);
    cache.relations.insert({1242, admin});

    //we create a new index for that
    persistor.create_index();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    client.request(methods::GET, "/tata/_count").then([] (http_response r) {
        r.content_ready().wait();
        std::cout << "val: " << r <<std::endl;
        BOOST_REQUIRE_EQUAL(r.status_code(), 200);
        auto json = r.extract_json().get();
        BOOST_REQUIRE_EQUAL(json["count"], js::value::number(0));
    }).wait();

    persistor.persist_admins();

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    // we should then have one admin
    client.request(methods::GET, "/tata/_count").then([] (http_response r) {
        BOOST_REQUIRE_EQUAL(r.status_code(), 200);
        auto json = r.extract_json().get();
        BOOST_REQUIRE_EQUAL(json["count"], js::value::number(1));
    }).wait();

    const auto check_elt = [](js::value& hit) {
        BOOST_CHECK_EQUAL(hit["_index"].as_string(), "tata");

        BOOST_CHECK_EQUAL(hit["_id"].as_string(), "admin:osm:1242"); //es id is a prefix + osmid
        BOOST_CHECK_EQUAL(hit["_source"]["id"].as_string(), "admin:osm:1242");

        BOOST_CHECK_EQUAL(hit["_source"]["level"].as_integer(), 42);
        BOOST_CHECK_EQUAL(hit["_source"]["name"].as_string(), "bob's bob");

        const auto zip_codes = hit["_source"]["zip_codes"].as_array();
        BOOST_CHECK_EQUAL(zip_codes.at(0).as_string(), "postal12");
        BOOST_CHECK_EQUAL(zip_codes.at(1).as_string(), "postal42");
        BOOST_CHECK_EQUAL(hit["_source"]["weight"].as_integer(), 0);

        //point are given in lon/lat
        BOOST_CHECK_EQUAL(hit["_source"]["coord"]["lat"].as_double(), 1.0);
        BOOST_CHECK_EQUAL(hit["_source"]["coord"]["lon"].as_double(), 2.0);

        //shapes are given in geojson
        const auto shape = hit["_source"]["shape"];
        BOOST_CHECK_EQUAL(shape.at("type").as_string(), "multipolygon");
        BOOST_REQUIRE_EQUAL(shape.at("coordinates").size(), 1);
        BOOST_REQUIRE_EQUAL(shape.at("coordinates").at(0).size(), 1);
        BOOST_REQUIRE_EQUAL(shape.at("coordinates").at(0).at(0).size(), 4);
        const auto poly = shape.at("coordinates").at(0).at(0);
        BOOST_REQUIRE_EQUAL(poly.at(0).size(), 2); //the lon/lat
        BOOST_CHECK_CLOSE(poly.at(0).at(0).as_double(), 2.0, 1e-5);
        BOOST_CHECK_CLOSE(poly.at(0).at(1).as_double(), 1.3, 1e-5);
        BOOST_CHECK_EQUAL(hit["_source"]["admin_shape"], shape); //the admin shape and the shape should be equals
    };

    //we look for all elt, we should only get the one we inserted
    client.request(methods::GET, "/tata/_search?q=*").then([&check_elt] (http_response r) {
        BOOST_REQUIRE_EQUAL(r.status_code(), 200);
        auto json = r.extract_json().get();
        BOOST_REQUIRE_EQUAL(json["hits"]["hits"].size(), 1);
        auto& hit = json["hits"]["hits"][0];
        check_elt(hit);
    }).wait();

    //then we look only for the one we want, by it's a portion of it's name
    web::http::uri_builder builder ("/tata/_search");
    builder.append_query("q", "bob's bob", true);
    client.request(methods::GET, builder.to_string()).then([&check_elt] (http_response r) {
        BOOST_REQUIRE_EQUAL(r.status_code(), 200);
        auto json = r.extract_json().get();
        BOOST_REQUIRE_EQUAL(json["hits"]["hits"].size(), 1);
        auto& hit = json["hits"]["hits"][0];
        check_elt(hit);
    }).wait();

    //just to be sure, we try with another portion of the name
    client.request(methods::GET, "/tata/_search?q=bob").then([&check_elt] (http_response r) {
        BOOST_REQUIRE_EQUAL(r.status_code(), 200);
        auto json = r.extract_json().get();
        BOOST_REQUIRE_EQUAL(json["hits"]["hits"].size(), 1);
        auto& hit = json["hits"]["hits"][0];
        check_elt(hit);
    }).wait();

    //and we try with a non existing name, we should be find anything
    client.request(methods::GET, "/tata/_search?q=bryan").then([&check_elt] (http_response r) {
        BOOST_REQUIRE_EQUAL(r.status_code(), 200);
        auto json = r.extract_json().get();
        BOOST_CHECK_EQUAL(json["hits"]["hits"].size(), 0);
    }).wait();
}

BOOST_AUTO_TEST_SUITE_END()

