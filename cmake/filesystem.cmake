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
    
    # Install busybox to the filesystem
    add_custom_target(
        install_busybox_fs
        DEPENDS create_fs_dirs busybox_build
        COMMAND ${CMAKE_SOURCE_DIR}/script/install_busybox.sh ${ROOT_DIR} ${BUSYBOX_INSTALL_DIR}
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
    # Create the filesystem image
    add_custom_target(
        create_ext4_image
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/disk_img
        COMMAND ${CMAKE_SOURCE_DIR}/script/create_filesystem.sh 
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
    create_fs_image(${ROOT_DIR} ${OUTPUT_IMAGE} ${SIZE_MB} 1)
    
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