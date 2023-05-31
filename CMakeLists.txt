cmake_minimum_required (VERSION 2.8)

project (demo)

find_library(YAMLCPP libyaml-cpp.a)
aux_source_directory(. SRC_LIST)

add_executable(main ${SRC_LIST})
target_link_libraries(main yaml-cpp)
