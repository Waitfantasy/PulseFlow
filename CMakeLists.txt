cmake_minimum_required(VERSION 3.10)
project(PulseFlow C)

set(CMAKE_C_STANDARD 11)

set(SOURCE_FILES
        php_PulseFlow.h
        PulseFlow.c
        tracing.h
        string_hash.h
        common.h)

add_executable(PulseFlow ${SOURCE_FILES})

add_custom_target(makefile COMMAND make && sudo make install WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})


include_directories(.)

set(PHP_SOURCE /usr/local/php/include/php)

include_directories(${PHP_SOURCE}/main)
include_directories(${PHP_SOURCE}/Zend)
include_directories(${PHP_SOURCE}/sapi)
include_directories(${PHP_SOURCE}/pear)
include_directories(${PHP_SOURCE})