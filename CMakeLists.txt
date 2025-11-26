cmake_minimum_required(VERSION 4.0)
project(undo_manager C)

set(CMAKE_C_STANDARD 11)

add_library(undo_manager SHARED undo_manager.c)
