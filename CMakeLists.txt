cmake_minimum_required(VERSION 3.10)
project(oscomp-dev C ASM)

# 创建cmake目录（如果不存在）
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/cmake)

# 确保lib目录存在
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include custom CMake modules
include(${CMAKE_SOURCE_DIR}/cmake/debug.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/filesystem.cmake)

# 首先处理第三方库
add_subdirectory(vendor)

# 然后处理内核
add_subdirectory(kernel)

# 定义输出目录
set(HOSTFS_DIR ${CMAKE_BINARY_DIR}/hostfs)
set(FS_IMAGE ${CMAKE_BINARY_DIR}/disk_img/rootfs.img)

# 确保busybox安装目录正确传递
get_directory_property(BUSYBOX_INSTALL_DIR DIRECTORY ${CMAKE_SOURCE_DIR}/vendor/busybox DEFINITION BUSYBOX_INSTALL_DIR)
if(NOT BUSYBOX_INSTALL_DIR)
    set(BUSYBOX_INSTALL_DIR ${CMAKE_BINARY_DIR}/vendor/busybox/install)
endif()

# 用户程序目录
set(USER_BUILD_DIR ${CMAKE_BINARY_DIR}/user)

# 添加系统镜像创建目标
add_system_image_target(
    create_system_image
    ${HOSTFS_DIR}
    ${FS_IMAGE}
    128
    ${BUSYBOX_INSTALL_DIR}
    ${USER_BUILD_DIR}
)

# 定义整体构建目标
add_custom_target(
    oskernel ALL
    DEPENDS riscv-pke create_system_image
    COMMENT "Building complete OS kernel with user programs and system image"
)

# 构建完成后的消息
add_custom_command(
    TARGET oskernel
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "OS Kernel build complete!"
    COMMAND ${CMAKE_COMMAND} -E echo "Filesystem image created at: ${FS_IMAGE}"
)

# 添加一个快速构建目标，不包括文件系统镜像
add_custom_target(
    quick_build
    DEPENDS riscv-pke
    COMMENT "Building OS kernel without filesystem image"
)

# Add debug information targets for the kernel
add_debug_info(riscv-pke)

# Add convenient individual debug targets
add_custom_target(
    kernel_disasm
    DEPENDS riscv-pke_disasm
    COMMENT "Generate kernel disassembly"
)

add_custom_target(
    kernel_symbols
    DEPENDS riscv-pke_symbols
    COMMENT "Generate kernel symbol table"
)

add_custom_target(
    kernel_debug_info
    DEPENDS riscv-pke_debug_all
    COMMENT "Generate all kernel debug information"
)