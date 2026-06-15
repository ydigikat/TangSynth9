# Native Linux build — no cross-compile
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_C_COMPILER gcc)
set(CMAKE_C_FLAGS "-O0 -g3 -Wall -Wextra -fprofile-arcs -ftest-coverage")