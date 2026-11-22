cmake_minimum_required(VERSION 3.16)

project(neko_meow LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto -funroll-loops")
    set(CMAKE_CXX_FLAGS_RELEASE_PORTABLE "-O3 -march=x86-64-v2 -flto")
endif()

find_package(Threads REQUIRED)

set(SRC_FILES
    src/main.cpp
    src/board.cpp
    src/nnue.cpp
)

add_executable(neko-meow ${SRC_FILES})

target_include_directories(neko-meow PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(neko-meow PRIVATE Threads::Threads)

# Install binary
install(TARGETS neko-meow DESTINATION bin)
