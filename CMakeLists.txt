cmake_minimum_required(VERSION 3.20.0)

project(main)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(prio REQUIRED)

# find_package(Boost REQUIRED COMPONENTS filesystem)

aux_source_directory(./src SRC)
add_executable(main ${SRC})
target_include_directories(main PUBLIC ${PRIO_INCLUDES})

# target_link_libraries(main Boost::filesystem)