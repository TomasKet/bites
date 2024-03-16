# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS ${EXTRA_COMPONENT_DIRS} $ENV{ADF_PATH}/components)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(bites)
