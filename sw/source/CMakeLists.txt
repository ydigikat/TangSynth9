#------------------------------------------------------------------------------
# Jason Wilden 20276
#------------------------------------------------------------------------------

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")

set(TARGET app.elf)

# Folders
set(SRC_DIR ${PROJECT_SOURCE_DIR}/source)
set(BSP_DIR ${SRC_DIR}/bsp)

# Main application
set(SRCS_APP
  ${SRC_DIR}/main.c 
  ${SRC_DIR}/controller/controller.c 
  ${SRC_DIR}/controller/midi.c
  ${SRC_DIR}/controller/voice.c
  ${SRC_DIR}/controller/env.c    
  ${SRC_DIR}/controller/env_luts.c    
)

set(INCL_APP ${SRC_DIR} ${SRC_DIR}/controller)
set(DEFS_APP 
      $<$<CONFIG:DEBUG>: DEBUG>       
      TRACE_ENABLED
)

# Size of the BSRAM in words on target FPGA.
# 16K Bytes = 2048 Words
set(BLOCK_SIZE 2048)

# Board support package & low/high level drivers
set(SRCS_BSP  
  ${BSP_DIR}/start.S  
  ${BSP_DIR}/runtime.c  
  ${BSP_DIR}/drv.c
)  

set(INCL_BSP ${BSP_DIR} )

# Build targets
add_executable(${TARGET} ${SRCS_APP} ${SRCS_BSP})
target_include_directories(${TARGET} PRIVATE ${INCL_APP} ${INCL_BSP})
target_compile_definitions(${TARGET} PRIVATE ${DEFS_APP} ${DEFS_BSP})
target_link_options(${TARGET} PRIVATE -T ${BSP_DIR}/linker.lds)
target_link_libraries(${TARGET} PRIVATE gcc)  

add_custom_command(TARGET ${TARGET} POST_BUILD
  COMMAND ${CMAKE_SIZE} ${TARGET}
  COMMAND ${CMAKE_OBJCOPY} -O binary ${TARGET} ${TARGET}.bin
  COMMAND ${CMAKE_OBJDUMP} -d -S ${TARGET} > ${TARGET}.s
  COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_CURRENT_BINARY_DIR} 
          python3 ${PROJECT_SOURCE_DIR}/tools/bin_to_hex.py ${TARGET}.bin ${BLOCK_SIZE}
  # Copy hex files to location expected by FPGA build
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/firmware_b0.hex ${PROJECT_SOURCE_DIR}/../hw/handoff/firmware_b0.hex 
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/firmware_b1.hex ${PROJECT_SOURCE_DIR}/../hw/handoff/firmware_b1.hex 
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/firmware_b2.hex ${PROJECT_SOURCE_DIR}/../hw/handoff/firmware_b2.hex 
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/firmware_b3.hex ${PROJECT_SOURCE_DIR}/../hw/handoff/firmware_b3.hex 
)

endif()