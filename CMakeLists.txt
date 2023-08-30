cmake_minimum_required(VERSION 3.20.0)

project(main)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

#一些优化开关
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2") # O2
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -mtune=native") # 针对当前机器的 CPU 架构和调整进行优化
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -funroll-loops") # 启用循环展开优化
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto") # 启用链接时优化
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -finline-functions") # 启用内联函数优化
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fomit-frame-pointer") # 不使用 frame pointer


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(prio REQUIRED)

# find_package(Boost REQUIRED COMPONENTS filesystem)

aux_source_directory(./src SRC)
add_executable(main ${SRC})
target_include_directories(main PUBLIC ${PRIO_INCLUDES})

# target_link_libraries(main Boost::filesystem)