# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# Add this line to disable the specific warning
add_compile_options(-Wno-missing-field-initializers)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# "Trim" the build. Include the minimal set of components, main, and anything it depends on.
idf_build_set_property(MINIMAL_BUILD ON)
# Keep component resolution reproducible per chip target. The repository
# contains multiple upstream board targets whose dependency graphs can differ.
idf_build_set_property(DEPENDENCIES_LOCK dependencies.lock.${IDF_TARGET})

if(DEFINED ENV{NARA_PROJECT_VER} AND NOT "$ENV{NARA_PROJECT_VER}" STREQUAL "")
    set(PROJECT_VER "$ENV{NARA_PROJECT_VER}")
else()
    set(PROJECT_VER "0.1.0")
endif()
project(nara_firmware)
