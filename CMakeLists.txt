cmake_minimum_required(VERSION 3.16)
project(TextEditor LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

add_executable(TextEditor src/main.cpp)
target_link_libraries(TextEditor ${CURSES_LIBRARIES})
target_compile_features(TextEditor PRIVATE cxx_std_17)

install(TARGETS TextEditor)