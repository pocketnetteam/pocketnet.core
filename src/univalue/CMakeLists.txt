cmake_minimum_required(VERSION 3.14)

# TODO version

project(univalue)

# TODO validate this works for windows

add_library(${PROJECT_NAME} include/univalue.h
                            lib/univalue.cpp
                            lib/univalue_get.cpp
                            lib/univalue_read.cpp
                            lib/univalue_write.cpp
                            lib/univalue_utffilter.h
                            lib/univalue_escapes.h
                            )
target_include_directories(${PROJECT_NAME} PUBLIC include/)