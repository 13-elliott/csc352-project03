cmake_minimum_required(VERSION 3.15)
project(project_03 C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(project_03
        mymalloc.c
        mymalloc.h
        mallocdrv.c)
