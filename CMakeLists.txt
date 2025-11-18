cmake_minimum_required(VERSION 4.0)
project(undo_manager_lib C)

set(CMAKE_C_STANDARD 11)

add_library(undo_manager_lib SHARED undo_manager.c)
