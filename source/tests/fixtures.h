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
#include "utils/logger.h"

#include <stdlib.h>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <thread>
#include <utils/exception.h>

#include <cpprest/http_client.h>
#include <cpprest/json.h>


using web::http::client::http_client;
using web::http::http_response;
using web::http::methods;
namespace js = web::json;
namespace ba = boost::algorithm;

auto logger = log4cplus::Logger::getInstance("log");

/**
 * Docker wrapper.
 *
 * The docker container is poped at creation and killed on destroy
 */
struct elastic_search_docker {
    elastic_search_docker() {
        auto uuid = boost::uuids::random_generator()();

        std::stringstream id;
        id << "hugin_elastic_tests-" << uuid;
        docker_id = id.str();
        std::stringstream cmd;
        // TODO don't forward port, but get the docker ip (but I think we should use the docker http client for those)
        // the port forwarding makes simultaneous tests impossible
        cmd << "docker run -d --name " << docker_id << " -p 9201:9200 elasticsearch";

        const auto cmd_str = cmd.str();
        LOG4CPLUS_INFO(logger, "starting a docker container");
        const auto cmd_res = system(cmd_str.c_str());
        if (cmd_res) {
            LOG4CPLUS_ERROR(log4cplus::Logger::getInstance("log"),
                            "cannot run docker command: " << cmd.str() << " return code: " << cmd_res);
            BOOST_FAIL("cannot run docker command");
        }

        container_started = true;
    }
    ~elastic_search_docker() {
        if (! container_started) { return; }
        //stop the docker on cleanup
        const auto cmd = "docker stop " + docker_id + " && docker rm " + docker_id;
        const auto res = system(cmd.c_str());
        if (res) {
            LOG4CPLUS_ERROR(log4cplus::Logger::getInstance("log"), "cannot stop docker: " << res << " inspect that manually");
        }
    }

    bool container_started = false;
    std::string docker_id;

};

struct logger_initialized {
    logger_initialized() { init_logger(); }
};

/**
 * Global test fixture, created only once for all the tests under the global fixture.
 *
 * If you need a fresh docker, create a new test file and use this as the global fixture
 */
struct es_global_fixture {
    logger_initialized log; // we put the logger init in a struct for it to be done before the docker stuffs
    elastic_search_docker docker;
    std::string elastic_search_url = "http://localhost:9201/";

    http_client client;
    es_global_fixture(): log(), client(elastic_search_url) {
        //we need to wait for the elastic search to pop
        poll_es();
    }

    void poll_es() {
        LOG4CPLUS_INFO(logger, "poll");
        // we query es until it gets started
        using namespace std::chrono;
        milliseconds dur(0);
        while (true) {
            try {
                //if the web server is not yet up, a std::exception is thrown by casablanca, we need to catch it
                const auto found = client.request(methods::GET, "/").then([] (http_response r) {
                    LOG4CPLUS_INFO(logger, "query; " << r.extract_string().get());
                    return r.status_code() == 200;
                }).get();

                if (found) { break; }
            } catch (const std::exception& e) {}

            if (dur > seconds(10)) {
                BOOST_FAIL("impossible to start elastics search");
            }

            LOG4CPLUS_INFO(logger, "we wait ");
            auto wait = milliseconds(500);
            std::this_thread::sleep_for(wait);
            dur += wait;
        }
        LOG4CPLUS_INFO(logger, "client up!!!");
    }
};


//helper to display http response
//only in tests, because slow
inline std::ostream& operator<< (std::ostream& os, const web::http::http_response& r) {
    // we wait to have the full response
    r.content_ready().wait();
    os << " -- " << r.to_string();
    os << "[" << r.status_code() << "]";
    if (r.status_code() != 200) {
        os << ": " << r.reason_phrase();
    }
    return os;
}