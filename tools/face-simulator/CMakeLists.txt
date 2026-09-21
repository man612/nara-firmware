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

find_package(SDL2 REQUIRED)

FetchContent_Declare(
  lvgl
  GIT_REPOSITORY https://github.com/lvgl/lvgl.git
  GIT_TAG v9.5.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(lvgl)

FetchContent_Declare(
  cjson
  GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
  GIT_TAG v1.7.19
  GIT_SHALLOW TRUE
)
set(ENABLE_CJSON_TEST OFF CACHE BOOL "" FORCE)
set(ENABLE_CJSON_UTILS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(cjson)

set(NARA_FACE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../main/display/nara_face")
set(NARA_PHYSICAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../main/physical")
set(NARA_VISION_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../main/vision")
set(NARA_OFFLINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../main/offline")

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
target_link_libraries(nara-face-simulator PRIVATE nara_face_view lvgl::lvgl SDL2::SDL2)

enable_testing()
add_executable(nara-face-controller-test tests/controller_test.cpp)
target_link_libraries(nara-face-controller-test PRIVATE nara_face_logic)
add_test(NAME nara-face-controller COMMAND nara-face-controller-test)

add_executable(nara-gesture-classifier-test
  tests/gesture_classifier_test.cpp
  "${NARA_PHYSICAL_DIR}/gesture_classifier.cc"
)
target_include_directories(nara-gesture-classifier-test PRIVATE "${NARA_PHYSICAL_DIR}")
add_test(NAME nara-gesture-classifier COMMAND nara-gesture-classifier-test)

add_executable(nara-vision-target-test
  tests/vision_target_tracker_test.cpp
  "${NARA_VISION_DIR}/vision_target_tracker.cc"
)
target_include_directories(nara-vision-target-test PRIVATE "${NARA_VISION_DIR}")
add_test(NAME nara-vision-target COMMAND nara-vision-target-test)

add_executable(nara-touch-classifier-test
  tests/touch_classifier_test.cpp
  "${NARA_PHYSICAL_DIR}/touch_classifier.cc"
)
target_include_directories(nara-touch-classifier-test PRIVATE "${NARA_PHYSICAL_DIR}")
add_test(NAME nara-touch-classifier COMMAND nara-touch-classifier-test)

add_executable(nara-offline-utility-test
  tests/offline_utility_state_test.cpp
  "${NARA_OFFLINE_DIR}/offline_utility_state.cc"
)
target_include_directories(nara-offline-utility-test PRIVATE "${NARA_OFFLINE_DIR}")
add_test(NAME nara-offline-utility COMMAND nara-offline-utility-test)

add_executable(nara-offline-capsule-test
  tests/offline_capsule_test.cpp
  "${NARA_OFFLINE_DIR}/offline_capsule.cc"
)
target_include_directories(
  nara-offline-capsule-test
  PRIVATE
    "${NARA_OFFLINE_DIR}"
    "${cjson_SOURCE_DIR}"
)
target_link_libraries(nara-offline-capsule-test PRIVATE cjson)
add_test(NAME nara-offline-capsule COMMAND nara-offline-capsule-test)
