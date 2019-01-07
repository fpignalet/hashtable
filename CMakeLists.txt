cmake_minimum_required(VERSION 3.9)
set(CMAKE_CXX_STANDARD 11)

project(hashtable_master)

add_executable(hashtable_master
        inc/hashfunc.h
        src/hashcore.c
        src/hashitem.c
        inc/hashcore.h
        tst/main.c
        src/murmur.c
        inc/murmur.h
        inc/test.h
        inc/timer.h)

add_definitions(-D__WITH_MURMUR -DTEST)