# Debug information generation module
# Place this file in a 'cmake' directory at the project root
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# Define a function to add debug information targets for an executable
function(add_debug_info TARGET_NAME)
    set(DEBUG_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/debug)
    
    # Create the debug directory
    add_custom_target(
        create_debug_dir
        COMMAND mkdir -p ${DEBUG_OUTPUT_DIRECTORY}
        COMMENT "Creating debug output directory"
    )
    
    # Get the full path to the target executable
    set(TARGET_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET_NAME})
    
    # Disassembly with source
    add_custom_target(
        ${TARGET_NAME}_disasm
        DEPENDS ${TARGET_NAME} create_debug_dir
        COMMAND riscv64-unknown-elf-objdump -D -S 
                "${TARGET_PATH}" > 
                ${DEBUG_OUTPUT_DIRECTORY}/${TARGET_NAME}.dump
        COMMENT "Generating disassembly with source for ${TARGET_NAME}"
    )
    
    # ELF analysis
    add_custom_target(
        ${TARGET_NAME}_elfinfo
        DEPENDS ${TARGET_NAME} create_debug_dir
        COMMAND riscv64-unknown-elf-readelf -a 
                "${TARGET_PATH}" > 
                ${DEBUG_OUTPUT_DIRECTORY}/${TARGET_NAME}.elfinfo
        COMMENT "Generating ELF information for ${TARGET_NAME}"
    )
    
    # Symbol table
    add_custom_target(
        ${TARGET_NAME}_symbols
        DEPENDS ${TARGET_NAME} create_debug_dir
        COMMAND riscv64-unknown-elf-nm -n -S 
                "${TARGET_PATH}" > 
                ${DEBUG_OUTPUT_DIRECTORY}/${TARGET_NAME}.symbols
        COMMENT "Generating symbol table for ${TARGET_NAME}"
    )
    
    # Global symbols
    add_custom_target(
        ${TARGET_NAME}_global_symbols
        DEPENDS ${TARGET_NAME} create_debug_dir
        COMMAND riscv64-unknown-elf-nm -g 
                "${TARGET_PATH}" > 
                ${DEBUG_OUTPUT_DIRECTORY}/${TARGET_NAME}.global-symbols
        COMMENT "Generating global symbol table for ${TARGET_NAME}"
    )
    
    # Header information
    add_custom_target(
        ${TARGET_NAME}_headers
        DEPENDS ${TARGET_NAME} create_debug_dir
        COMMAND riscv64-unknown-elf-objdump --all-headers 
                "${TARGET_PATH}" > 
                ${DEBUG_OUTPUT_DIRECTORY}/${TARGET_NAME}.headers
        COMMENT "Generating header information for ${TARGET_NAME}"
    )
    
    # Section information
    add_custom_target(
        ${TARGET_NAME}_sections
        DEPENDS ${TARGET_NAME} create_debug_dir
        COMMAND riscv64-unknown-elf-objdump -h 
                "${TARGET_PATH}" > 
                ${DEBUG_OUTPUT_DIRECTORY}/${TARGET_NAME}.sections
        COMMENT "Generating section information for ${TARGET_NAME}"
    )
    
    # String dump
    add_custom_target(
        ${TARGET_NAME}_strings
        DEPENDS ${TARGET_NAME} create_debug_dir
        COMMAND riscv64-unknown-elf-strings 
                "${TARGET_PATH}" > 
                ${DEBUG_OUTPUT_DIRECTORY}/${TARGET_NAME}.strings
        COMMENT "Extracting strings from ${TARGET_NAME}"
    )
    
    # Relocation information
    add_custom_target(
        ${TARGET_NAME}_relocations
        DEPENDS ${TARGET_NAME} create_debug_dir
        COMMAND riscv64-unknown-elf-objdump -r -d 
                "${TARGET_PATH}" > 
                ${DEBUG_OUTPUT_DIRECTORY}/${TARGET_NAME}.reloc
        COMMENT "Generating relocation information for ${TARGET_NAME}"
    )
    
    # Dynamic information
    add_custom_target(
        ${TARGET_NAME}_dynamic
        DEPENDS ${TARGET_NAME} create_debug_dir
        COMMAND riscv64-unknown-elf-readelf -d 
                "${TARGET_PATH}" > 
                ${DEBUG_OUTPUT_DIRECTORY}/${TARGET_NAME}.dynamic
        COMMENT "Generating dynamic information for ${TARGET_NAME}"
    )
    
    # Size information
    add_custom_target(
        ${TARGET_NAME}_size
        DEPENDS ${TARGET_NAME} create_debug_dir
        COMMAND riscv64-unknown-elf-size 
                "${TARGET_PATH}" > 
                ${DEBUG_OUTPUT_DIRECTORY}/${TARGET_NAME}.size
        COMMENT "Generating size information for ${TARGET_NAME}"
    )
    
    # Meta target to generate all debug info
    add_custom_target(
        ${TARGET_NAME}_debug_all
        DEPENDS 
            ${TARGET_NAME}_disasm
            ${TARGET_NAME}_elfinfo
            ${TARGET_NAME}_symbols
            ${TARGET_NAME}_global_symbols
            ${TARGET_NAME}_headers
            ${TARGET_NAME}_sections
            ${TARGET_NAME}_strings
            ${TARGET_NAME}_relocations
            ${TARGET_NAME}_dynamic
            ${TARGET_NAME}_size
        COMMENT "Generating all debug information for ${TARGET_NAME}"
    )
endfunction()