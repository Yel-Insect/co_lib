cmake_minimum_required (VERSION 2.8)

project (demo)

find_library(YAMLCPP libyaml-cpp.a)
set(CMAKE_CXX_FLAGS   "-g")
set(SRC_LIST http/http11_parser.cc
	http/httpclient_parser.cc )
aux_source_directory(. SRC_LIST)

add_executable(main ${SRC_LIST})
target_link_libraries(main yaml-cpp)
