# Filesystem image creation module
# Place this file in a 'cmake' directory at the project root

function(create_directory_structure ROOT_DIR)
    # Create the basic directory structure for the filesystem
    add_custom_target(
        create_fs_dirs
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ROOT_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ROOT_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ROOT_DIR}/sbin
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ROOT_DIR}/etc
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ROOT_DIR}/lib
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ROOT_DIR}/usr/bin
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ROOT_DIR}/usr/lib
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ROOT_DIR}/usr/sbin
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ROOT_DIR}/dev
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ROOT_DIR}/proc
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ROOT_DIR}/sys
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ROOT_DIR}/tmp
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ROOT_DIR}/var
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ROOT_DIR}/root
        COMMENT "Creating basic directory structure for filesystem"
    )
endfunction()

# function(create_basic_config_files ROOT_DIR)
#     # Create configuration files
#     add_custom_target(
#         create_fs_configs
#         DEPENDS create_fs_dirs
#         # Basic configuration files
#         COMMAND bash -c "echo '/bin/sh' > ${ROOT_DIR}/etc/shells"
#         COMMAND bash -c "echo 'root::0:0:root:/root:/bin/sh' > ${ROOT_DIR}/etc/passwd"
#         COMMAND bash -c "echo 'root:x:0:' > ${ROOT_DIR}/etc/group"
#         COMMAND bash -c "echo '::sysinit:/etc/init.d/rcS' > ${ROOT_DIR}/etc/inittab"
#         COMMAND bash -c "echo '::respawn:-/bin/sh' >> ${ROOT_DIR}/etc/inittab"
#         COMMAND bash -c "echo '::restart:/sbin/init' >> ${ROOT_DIR}/etc/inittab"
#         COMMAND bash -c "echo '::shutdown:/bin/umount -a -r' >> ${ROOT_DIR}/etc/inittab"
        
#         # Init scripts
#         COMMAND ${CMAKE_COMMAND} -E make_directory ${ROOT_DIR}/etc/init.d
#         COMMAND bash -c "echo '#!/bin/sh' > ${ROOT_DIR}/etc/init.d/rcS"
#         COMMAND bash -c "echo 'mount -t proc none /proc' >> ${ROOT_DIR}/etc/init.d/rcS"
#         COMMAND bash -c "echo 'mount -t sysfs none /sys' >> ${ROOT_DIR}/etc/init.d/rcS"
#         COMMAND bash -c "echo 'mount -t devtmpfs none /dev' >> ${ROOT_DIR}/etc/init.d/rcS"
#         COMMAND chmod +x ${ROOT_DIR}/etc/init.d/rcS
#         COMMENT "Creating basic configuration files for filesystem"
#     )
# endfunction()

function(install_busybox ROOT_DIR BUSYBOX_INSTALL_DIR)
    # Create a helper script for installing busybox
    file(WRITE ${CMAKE_BINARY_DIR}/install_busybox.sh
"#!/bin/bash
ROOT_DIR=\"$1\"
BUSYBOX_DIR=\"$2\"

# Copy bin directory if it exists
if [ -d \"$BUSYBOX_DIR/bin\" ]; then
    cp -r \"$BUSYBOX_DIR/bin\"/* \"$ROOT_DIR/bin/\" || echo \"Failed to copy bin directory\"
else
    echo \"Busybox bin directory not found\"
fi

# Copy sbin directory if it exists
if [ -d \"$BUSYBOX_DIR/sbin\" ]; then
    cp -r \"$BUSYBOX_DIR/sbin\"/* \"$ROOT_DIR/sbin/\" || echo \"Failed to copy sbin directory\"
else
    echo \"Busybox sbin directory not found\"
fi

# Copy usr/bin directory if it exists
if [ -d \"$BUSYBOX_DIR/usr/bin\" ]; then
    cp -r \"$BUSYBOX_DIR/usr/bin\"/* \"$ROOT_DIR/usr/bin/\" || echo \"Failed to copy usr/bin directory\"
else
    echo \"usr/bin directory not found, skipping\"
fi

# Copy usr/sbin directory if it exists
if [ -d \"$BUSYBOX_DIR/usr/sbin\" ]; then
    cp -r \"$BUSYBOX_DIR/usr/sbin\"/* \"$ROOT_DIR/usr/sbin/\" || echo \"Failed to copy usr/sbin directory\"
else
    echo \"usr/sbin directory not found, skipping\"
fi
"
    )
    
    # Make the script executable
    execute_process(COMMAND chmod +x ${CMAKE_BINARY_DIR}/install_busybox.sh)
    
    # Install busybox to the filesystem
    add_custom_target(
        install_busybox_fs
        DEPENDS create_fs_dirs busybox_build
        COMMAND ${CMAKE_BINARY_DIR}/install_busybox.sh ${ROOT_DIR} ${BUSYBOX_INSTALL_DIR}
        COMMENT "Installing busybox to filesystem"
    )
endfunction()

function(install_user_programs ROOT_DIR USER_BUILD_DIR)
    # Install user programs to the filesystem
    add_custom_target(
        install_user_programs_fs
        DEPENDS create_fs_dirs
        COMMAND bash -c "if [ -d '${USER_BUILD_DIR}' ]; then ${CMAKE_COMMAND} -E copy_directory ${USER_BUILD_DIR} ${ROOT_DIR}/bin; else echo 'User programs directory not found'; fi"
        COMMENT "Installing user programs to filesystem"
    )
endfunction()

function(create_fs_image ROOT_DIR OUTPUT_IMAGE SIZE_MB FORCE_REBUILD)
    # Create a helper script for creating the filesystem image
    file(WRITE ${CMAKE_BINARY_DIR}/create_filesystem.sh
"#!/bin/bash
# 创建文件系统镜像脚本 - 简化版本
set -e

SOURCE_DIR=\"$1\"
OUTPUT_IMG=\"$2\"
SIZE_MB=\"$3\"

# 创建输出目录
mkdir -p \"$(dirname \"$OUTPUT_IMG\")\"

echo \"创建 $SIZE_MB MB 大小的 ext4 文件系统镜像...\"

# 创建空白镜像文件
dd if=/dev/zero of=\"$OUTPUT_IMG\" bs=1M count=\"$SIZE_MB\"

# 创建 ext4 文件系统
mkfs.ext4 -F \"$OUTPUT_IMG\"

# 创建挂载点
MOUNT_POINT=\"/tmp/fs_mount_temp\"
mkdir -p \"$MOUNT_POINT\"

# # 挂载文件系统
# echo \"挂载镜像...\"
# sudo mount -o loop \"$OUTPUT_IMG\" \"$MOUNT_POINT\"

# # 复制文件
# echo \"复制文件到镜像...\"
# sudo cp -a \"$SOURCE_DIR\"/* \"$MOUNT_POINT\"/

# # 设置权限
# echo \"设置权限...\"
# sudo chown -R root:root \"$MOUNT_POINT\"
# sudo chmod 755 \"$MOUNT_POINT\"

# # 卸载文件系统
# echo \"卸载镜像...\"
# sudo umount \"$MOUNT_POINT\"

# # 清理挂载点
# rmdir \"$MOUNT_POINT\"

echo \"文件系统镜像创建完成: $OUTPUT_IMG\"
"
    )
    
    # Make the script executable
    execute_process(COMMAND chmod +x ${CMAKE_BINARY_DIR}/create_filesystem.sh)
    
    # Create the filesystem image
    add_custom_target(
        create_ext4_image
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/disk_img
        COMMAND ${CMAKE_BINARY_DIR}/create_filesystem.sh 
                ${ROOT_DIR} 
                ${OUTPUT_IMAGE} 
                ${SIZE_MB}
        COMMENT "Creating ext4 filesystem image of ${SIZE_MB}MB from ${ROOT_DIR}"
    )
endfunction()


# Complete system image target
function(add_system_image_target 
         TARGET_NAME 
         ROOT_DIR 
         OUTPUT_IMAGE 
         SIZE_MB 
         BUSYBOX_DIR 
         USER_DIR)
    
    # Create directory structure
    create_directory_structure(${ROOT_DIR})
    
    # Create basic configuration files
    # create_basic_config_files(${ROOT_DIR})
    
    # Install busybox
    install_busybox(${ROOT_DIR} ${BUSYBOX_DIR})
    
    # # Install user programs
    # install_user_programs(${ROOT_DIR} ${USER_DIR})
    
    # Create filesystem image
    create_fs_image(${ROOT_DIR} ${OUTPUT_IMAGE} ${SIZE_MB} 0)
    
    # Add dependencies to ensure proper order
    # add_dependencies(create_fs_configs create_fs_dirs)
    add_dependencies(install_busybox_fs create_fs_dirs)
    # add_dependencies(install_user_programs_fs create_fs_dirs)
    # add_dependencies(create_ext4_image create_fs_configs install_busybox_fs install_user_programs_fs)
    add_dependencies(create_ext4_image install_busybox_fs )

    
    # Meta target to create the complete system image
    add_custom_target(
        ${TARGET_NAME}
        DEPENDS create_ext4_image
        COMMENT "Creating complete system image"
    )
endfunction()