#############################################################
#                                                           #
#               CMakeLists.txt for gateway_server           #
#                 Edit by stanjiang 2024.07.17              #
#############################################################

cmake_minimum_required(VERSION 3.10)
project(GatewayServer)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)

# 添加依赖库
find_package(spdlog REQUIRED)
find_package(Protobuf REQUIRED)

# 设置 libuv 的路径
set(LIBUV_INCLUDE_DIR "/usr/local/include")
set(LIBUV_LIBRARY "/usr/local/lib/libuv.so")

# 包含头文件目录
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/../common
    ${PROJECT_SOURCE_DIR}/../proto/include
    ${PROJECT_SOURCE_DIR}/../proto/include/cs_proto
    ${PROJECT_SOURCE_DIR}/../proto/include/ss_proto
    ${Protobuf_INCLUDE_DIRS}
    ${LIBUV_INCLUDE_DIR}
)

# 查找所有源文件
file(GLOB COMMON_SOURCES "${PROJECT_SOURCE_DIR}/../common/*.cpp")
file(GLOB SERVER_SOURCES "${PROJECT_SOURCE_DIR}/*.cpp")
file(GLOB CS_PROTO_SOURCES "${PROJECT_SOURCE_DIR}/../proto/include/cs_proto/*.cpp")
file(GLOB SS_PROTO_SOURCES "${PROJECT_SOURCE_DIR}/../proto/include/ss_proto/*.cpp")

# 添加可执行文件
add_executable(gateway_server
    ${SERVER_SOURCES}
    ${COMMON_SOURCES}
    ${CS_PROTO_SOURCES}
    ${SS_PROTO_SOURCES}
)

# 链接spdlog、protobuf和libuv库
target_link_libraries(gateway_server
    spdlog::spdlog
    ${Protobuf_LIBRARIES}
    ${LIBUV_LIBRARY}
)

# 添加编译选项
target_compile_options(gateway_server PRIVATE -Wall -Wextra -Werror -g)

# 设置输出目录
set_target_properties(gateway_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
)