#include "conf.h"

namespace navitia { namespace config {

const char* fixtures_dir = "${FIXTURES_DIR}";
const char* build_type = "${CMAKE_BUILD_TYPE}";
const char* project_version = "${GIT_REVISION}";

}}// namespace navitia::config
