cmake_minimum_required(VERSION 3.30)
project(api_set_resolver)

set(CMAKE_CXX_STANDARD 14)

add_library(api_set_resolver INTERFACE ApiSetResolver.hpp)

set_target_properties(api_set_resolver PROPERTIES LINKER_LANGUAGE CXX)
