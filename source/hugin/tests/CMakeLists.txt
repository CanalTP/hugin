SET(BOOST_LIBS ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

add_executable(hugin_tests hugin_tests.cpp)

target_link_libraries(hugin_tests hugin_lib utils osmpbf ${CASABLANCA_LIBRARY} ${BOOST_LIBS} log4cplus z protobuf)
ADD_BOOST_TEST(hugin_tests)
