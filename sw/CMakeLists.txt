#------------------------------------------------------------------------------
# Jason Wilden 2025
#------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.20)

PROJECT("app" C ASM)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Only add tests when building for host
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")    
    enable_testing()
    add_subdirectory(test)        
endif()

add_subdirectory(source)