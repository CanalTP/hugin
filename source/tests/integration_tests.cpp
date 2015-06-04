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
    admin.postal_codes.insert("postal42");
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

    //we look for all elt, we should only get the one we inserted
    client.request(methods::GET, "/tata/_search?q=*.*").then([] (http_response r) {
        BOOST_REQUIRE_EQUAL(r.status_code(), 200);
        auto json = r.extract_json().get();
        BOOST_REQUIRE_EQUAL(json["hits"]["hits"].size(), 1);
        BOOST_CHECK_EQUAL(json["hits"]["hits"][0]["_id"].as_string(), "admin:1242"); //es id is a prefix + osmid
        BOOST_CHECK_EQUAL(json["hits"]["hits"][0]["_index"].as_string(), "tata");
        BOOST_CHECK_EQUAL(json["hits"]["hits"][0]["_source"]["coord"].as_string(), "POINT(1.000000 2.000000)");
        BOOST_CHECK_EQUAL(json["hits"]["hits"][0]["_source"]["level"].as_integer(), 42);
        BOOST_CHECK_EQUAL(json["hits"]["hits"][0]["_source"]["name"].as_string(), "bob's bob");
        BOOST_CHECK_EQUAL(json["hits"]["hits"][0]["_source"]["post_code"].as_string(), "postal12-postal42");
        BOOST_CHECK(ba::starts_with(json["hits"]["hits"][0]["_source"]["shape"].as_string(), "MULTIPOLYGON(("));
        BOOST_CHECK(ba::starts_with(json["hits"]["hits"][0]["_source"]["admin_shape"].as_string(), "MULTIPOLYGON(("));
        BOOST_CHECK_EQUAL(json["hits"]["hits"][0]["_source"]["uri"].as_string(), "admin:1242");
        BOOST_CHECK_EQUAL(json["hits"]["hits"][0]["_source"]["weight"].as_integer(), 0);
    }).wait();
}

BOOST_AUTO_TEST_SUITE_END()

