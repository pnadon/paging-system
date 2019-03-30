cmake_minimum_required(VERSION 3.13)
project(paginSystem C)

set(CMAKE_C_STANDARD 99)

add_executable(paginSystem pagingSystem.c pagingSystem.h serialPager.c serialPager.h)