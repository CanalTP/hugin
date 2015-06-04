SET(BOOST_LIBS ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

add_executable(integration_tests integration_tests.cpp)

target_link_libraries(integration_tests hugin_lib utils osmpbf ${CASABLANCA_LIBRARY} ${BOOST_LIBS} log4cplus z protobuf)
ADD_BOOST_TEST(integration_tests)
