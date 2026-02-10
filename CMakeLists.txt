cmake_minimum_required(VERSION 3.10.0)

project(cunzip)

file(GLOB_RECURSE SRC_FILES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
)

find_package(spdlog CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

add_executable(cunzip ${SRC_FILES})

target_include_directories(cunzip PUBLIC include)
target_link_libraries(cunzip PRIVATE spdlog::spdlog ZLIB::ZLIB)