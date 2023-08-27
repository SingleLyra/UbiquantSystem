cmake_minimum_required(VERSION 3.20.0)

project(main)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# find_package(Boost REQUIRED COMPONENTS filesystem)

add_executable(main main.cpp
        readlogs.h readlogs.cpp
        )
# target_link_libraries(main Boost::filesystem)