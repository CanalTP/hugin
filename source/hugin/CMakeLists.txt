SET(BOOST_LIBS ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY} )


find_package(Casablanca)

add_executable(hugin hugin.cpp rubber.h rubber.cpp persistor.cpp persistor.h types.cpp types.h)

target_link_libraries(hugin utils osmpbf ${CASABLANCA_LIBRARY} ${BOOST_LIBS} log4cplus z protobuf)
