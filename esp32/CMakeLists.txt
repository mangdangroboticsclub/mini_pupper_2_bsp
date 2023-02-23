# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/system/console/advanced/components)
if(CMAKE_HOST_WIN32)
  set(PROJECT_VER "Build on Windows")
ELSE()
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/get-version.sh OUTPUT_VARIABLE PROJECT_VER)
ENDIF()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(mini_pupper_app)
