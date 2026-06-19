cmake_minimum_required(VERSION 3.20)

# --------------------------------------------------------
# Toolchain (baremetal)
# --------------------------------------------------------
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv32)

# Tool prefix
set(TOOLS_PREFIX "riscv32-unknown-elf-")

# Toolchain location
set(DEFAULT_TOOLS_DIR "/opt/riscv32i/bin/")
set(TOOLS_DIR "${DEFAULT_TOOLS_DIR}")

# --------------------------------------------------------
# Tool executables
# --------------------------------------------------------
set(CMAKE_C_COMPILER     ${TOOLS_DIR}${TOOLS_PREFIX}gcc)
set(CMAKE_ASM_COMPILER   ${TOOLS_DIR}${TOOLS_PREFIX}gcc)
set(CMAKE_CXX_COMPILER   ${TOOLS_DIR}${TOOLS_PREFIX}g++)
set(CMAKE_OBJCOPY        ${TOOLS_DIR}${TOOLS_PREFIX}objcopy)
set(CMAKE_SIZE           ${TOOLS_DIR}${TOOLS_PREFIX}size)


# --------------------------------------------------------
# Compiler flags (non-board-specific)
# --------------------------------------------------------
set(CMAKE_C_FLAGS "-march=rv32im -mabi=ilp32 -Os -g3 -Wall -Wextra -ffunction-sections -fdata-sections -ffreestanding -nostdlib")
set(CMAKE_ASM_FLAGS   "-march=rv32im -mabi=ilp32 -ffunction-sections -fdata-sections")
set(CMAKE_EXE_LINKER_FLAGS "-march=rv32im -mabi=ilp32 -nostdlib -nostartfiles -Wl,--gc-sections -Wl,-Map,firmware.map")

# Disable try-compile attempts for cross builds
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
