cmake_minimum_required(VERSION 3.10)
project(kernel C ASM)

# 设置 RISC-V 交叉编译工具链
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv64)
# riscv64-unknown-elf
# set(CMAKE_C_COMPILER riscv64-unknown-elf-gcc)
# set(CMAKE_CXX_COMPILER riscv64-unknown-elf-g++)
# set(CMAKE_ASM_COMPILER riscv64-unknown-elf-gcc)
# riscv64-linux-gnu-gcc
set(CMAKE_C_COMPILER riscv64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER riscv64-linux-gnu-g++)
set(CMAKE_ASM_COMPILER riscv64-linux-gnu-gcc)

# 编译标志
set(CMAKE_C_FLAGS "-Wall -Werror -Wl,-Map=kernel.map -nostdlib -nostartfiles -gdwarf-3 -fno-builtin -nostartfiles -nostdinc -ffreestanding -D__NO_INLINE__ -mcmodel=medany -g -Og -std=gnu99 -Wno-unused -Wno-attributes -Wl,--no-relax -Wl,--build-id=none -Wl,-z,notext -fno-delete-null-pointer-checks -fno-PIE -fno-pic -fno-omit-frame-pointer -lgcc")
# 设置静态链接
set(CMAKE_EXE_LINKER_FLAGS "-static  -Xlinker -T ${CMAKE_SOURCE_DIR}/kernel/kernel.lds" CACHE STRING "Static linking flags" FORCE)
# Apply the same flags to ASM
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS}")

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 递归获取 kernel 目录及其所有子目录中的 C 源文件
file(GLOB_RECURSE KERNEL_SOURCES "**/*.c")
file(GLOB_RECURSE KERNEL_SOURCES "*.c")

# 递归获取 kernel 目录及其所有子目录中的 汇编（.S）源文件
file(GLOB_RECURSE KERNEL_ASM_SOURCES "**/*.S")
file(GLOB_RECURSE KERNEL_ASM_SOURCES "*.S")

add_executable(kernel.elf ${KERNEL_SOURCES} ${KERNEL_ASM_SOURCES})

# 修改后的头文件包含路径，只保留include/和vendor/musl/include/
target_include_directories(kernel.elf PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/vendor/musl/include
    ${CMAKE_SOURCE_DIR}/vendor/musl/arch/generic
    ${CMAKE_SOURCE_DIR}/vendor/musl/arch/riscv64
    ${CMAKE_SOURCE_DIR}/vendor/musl/obj/include
    ${CMAKE_SOURCE_DIR}/vendor/musl/temp_install/include
)

# 关键修改: 确保内核依赖于musl库和lwext4库的构建
# 需要明确依赖musl_build目标，因为它是ExternalProject创建的实际构建目标
add_dependencies(kernel.elf musl_build lwext4)

# 直接链接库文件，确保它们已经构建完成
target_link_libraries(kernel.elf PRIVATE 
    ${CMAKE_BINARY_DIR}/lib/liblwext4.a
    # ${CMAKE_BINARY_DIR}/lib/libc.a

)