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
#include "utils/logger.h"

#include <stdlib.h>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/algorithm/string.hpp>

#include <thread>
#include <utils/exception.h>

struct logger_initialized {
    logger_initialized() { init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized)

/**
 * The docker container will be poped once every test suite
 *
 * If you need a clean docker (beware it will cost some time to start elastic search),
 * just create a new test suite:
 * BOOST_FIXTURE_TEST_SUITE(my_new_test_suite, elastic_search_docker)
 */
struct elastic_search_docker {
    elastic_search_docker() {
        auto uuid = boost::uuids::random_generator()();

        std::stringstream id;
        id << "hugin_elastic_tests-" << uuid;
        docker_id = id.str();
        std::stringstream cmd;
        // TODO don't forward port, but get the docker ip (but I think we should use the docker http client for those)
        //the port forwarding makes simultaneous tests impossible
        cmd << "docker run -d --name " << docker_id << " -p 9201:9200 elasticsearch";

        const auto cmd_str = cmd.str();
        const auto cmd_res = system(cmd_str.c_str());
        if (cmd_res) {
            LOG4CPLUS_ERROR(log4cplus::Logger::getInstance("log"),
                            "cannot run docker command: " << cmd.str() << " return code: " << cmd_res);
            throw navitia::exception("cannot run docker command");
        }
        container_started = true;
    }
    ~elastic_search_docker() {
        if (! container_started) { return; }
        //stop the docker on cleanup
        const auto cmd = "docker stop " + docker_id + " && docker rm " + docker_id;
        const auto res = system(cmd.c_str());
        if (res) {
            LOG4CPLUS_ERROR(log4cplus::Logger::getInstance("log"), "cannot stop docker: " << res);
        }
    }
    bool container_started = false;
    std::string docker_id;
};

BOOST_FIXTURE_TEST_SUITE(simple_elastic_import, elastic_search_docker)

BOOST_AUTO_TEST_CASE(add_one_admin)
{
    std::cout << "hey joe ?!" << std::endl;
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()
