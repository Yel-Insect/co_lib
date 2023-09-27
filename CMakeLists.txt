cmake_minimum_required(VERSION 3.15)

project(co_lib)
find_library(YAMLCPP libyaml-cpp.a)
set(CMAKE_CXX_FLAGS   "-g")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

# aux_source_directory(./src DIR_SRCS)
# aux_source_directory(./test DIR_SRCS)
include_directories(./include)
include_directories(./include/http)
include_directories(./include/http-parser)

#LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/lib/http_parser)

FILE(GLOB_RECURSE SRC_LIST ${CMAKE_SOURCE_DIR}/src/*.cc)
FILE(GLOB TEST_SRCS ${CMAKE_SOURCE_DIR}/test/*.cc)

message(${SRC_LIST})
message(${TEST_SRCS})

set(LIB_LIST yaml-cpp)

add_library(lib STATIC ${SRC_LIST})
target_link_libraries(lib ${LIB_LIST})

FOREACH(test_src ${TEST_SRCS})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} lib)
ENDFOREACH(test_src)


#add_executable(co_lib ${DIR_SRCS})



