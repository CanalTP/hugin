SET(BOOST_LIBS ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY} )

SET(HUGIN_FILES rubber.h rubber.cpp persistor.cpp persistor.h types.cpp types.h)
add_library(hugin_lib ${HUGIN_FILES} visitors.h visitors.cpp)
add_executable(hugin hugin.cpp)

target_link_libraries(hugin hugin_lib utils osmpbf ${CASABLANCA_LIBRARY} ${BOOST_LIBS} log4cplus z protobuf)
