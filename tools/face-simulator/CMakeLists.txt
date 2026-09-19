cmake_minimum_required(VERSION 3.20)
project(nara_face_simulator LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Keep desktop LVGL aligned with the embedded firmware line.
set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE PATH "" FORCE)
set(LV_BUILD_SET_CONFIG_OPTS ON CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)
set(LV_BUILD_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  lvgl
  GIT_REPOSITORY https://github.com/lvgl/lvgl.git
  GIT_TAG v9.5.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(lvgl)

set(NARA_FACE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../main/display/nara_face")

add_library(nara_face_logic
  "${NARA_FACE_DIR}/nara_face_controller.cc"
)
target_include_directories(nara_face_logic PUBLIC "${NARA_FACE_DIR}")

add_library(nara_face_view
  "${NARA_FACE_DIR}/nara_face_view.cc"
)
target_include_directories(nara_face_view PUBLIC "${NARA_FACE_DIR}")
target_link_libraries(nara_face_view PUBLIC nara_face_logic lvgl::lvgl)

add_executable(nara-face-simulator main.cpp)
target_link_libraries(nara-face-simulator PRIVATE nara_face_view lvgl::lvgl)

enable_testing()
add_executable(nara-face-controller-test tests/controller_test.cpp)
target_link_libraries(nara-face-controller-test PRIVATE nara_face_logic)
add_test(NAME nara-face-controller COMMAND nara-face-controller-test)
