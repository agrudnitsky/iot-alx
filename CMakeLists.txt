cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENT_DIRS $ENV{FASTLED_DIR})
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(iot-alx)
message("ecd: " ${EXTRA_COMPONENT_DIRS})
