#############################################################
#                                                           #
#                CMakeLists.txt for tcpclient               #
#                  Edit by stanjiang 2024.07.10             #
#############################################################

cmake_minimum_required(VERSION 3.10)
project(TcpClient)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)

# 添加spdlog和protobuf依赖
find_package(spdlog REQUIRED)
find_package(Protobuf REQUIRED)

# 设置 libuv 的路径
set(LIBUV_INCLUDE_DIR "/usr/local/include")
set(LIBUV_LIBRARY "/usr/local/lib/libuv.so")

# 设置 librdkafka 的路径
set(RDKAFKA_INCLUDE_DIR "/usr/include/librdkafka")
set(RDKAFKA_LIBRARY "/usr/lib/x86_64-linux-gnu/librdkafka++.so")

# 包含头文件目录
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/../common
    ${PROJECT_SOURCE_DIR}/../proto/include
    ${PROJECT_SOURCE_DIR}/../proto/include/cs_proto
    ${Protobuf_INCLUDE_DIRS}
    ${LIBUV_INCLUDE_DIR}
    ${RDKAFKA_INCLUDE_DIR}
)

# 查找所有源文件
file(GLOB COMMON_SOURCES "${PROJECT_SOURCE_DIR}/../common/*.cpp")
file(GLOB CLIENT_SOURCES "${PROJECT_SOURCE_DIR}/*.cpp")
file(GLOB PROTO_SOURCES "${PROJECT_SOURCE_DIR}/../proto/include/cs_proto/*.cpp")

# 添加可执行文件
add_executable(tcp_client
    ${CLIENT_SOURCES}
    ${COMMON_SOURCES}
    ${PROTO_SOURCES}
)

# 链接spdlog和protobuf库
target_link_libraries(tcp_client
    spdlog::spdlog
    ${Protobuf_LIBRARIES}
    ${LIBUV_LIBRARY}
    ${RDKAFKA_LIBRARY}
)

# 添加编译选项
target_compile_options(tcp_client PRIVATE -Wall -Wextra -Werror -g)
