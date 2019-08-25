cmake_minimum_required(VERSION 3.8)

include(../sfinder-cpp/build/vars/sfinder_core.cmake)
include(./vars/sfinder_exp_main.cmake)

project(sfinder-exp-main)

# Compiler options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-m64 -Wall")
#set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")  # -march=native -s
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto -march=native -DNDEBUG")

# Build defines
include_directories(${sfinder_core_SOURCE_DIR})

add_executable(${PROJECT_NAME} ${sfinder_exp_main_SRC})

target_link_libraries(${PROJECT_NAME}
        sfinder_core_mac
        )