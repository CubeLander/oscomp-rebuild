cmake_minimum_required(VERSION 3.10)

# 第三方库构建配置
message(STATUS "Configuring third-party libraries")

# 确保构建输出目录存在
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# musl库 - 使用ExternalProject构建
add_subdirectory(musl)

# lwext4库 - 使用脚本构建
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/lib/liblwext4.a
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/lib
    COMMAND bash ${CMAKE_SOURCE_DIR}/script/build-lwext4.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Building lwext4 library"
    VERBATIM
)

add_custom_target(lwext4 DEPENDS ${CMAKE_BINARY_DIR}/lib/liblwext4.a)

# busybox - 必需
add_subdirectory(busybox)

# 创建一个整合目标，可以一次性构建所有第三方库
add_custom_target(
    build_all_vendor
    DEPENDS lwext4 musl_build busybox_build
    COMMENT "Building all third-party libraries"
)

# 确保库构建目录存在
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/lib)