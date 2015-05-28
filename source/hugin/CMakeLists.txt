SET(BOOST_LIBS ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY} )



add_executable(hugin hugin.cpp rubber.h)
message("pb lib is : ${PROTOBUF_LIBRARY}")
target_link_libraries(hugin utils osmpbf ${BOOST_LIBS} log4cplus z protobuf)
