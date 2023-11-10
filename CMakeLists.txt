cmake_minimum_required(VERSION 3.10)
project(DSBufMgr)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_subdirectory(src)
aux_source_directory(. DIR_SRCS)
add_executable(DSBufMgr ${DIR_SRCS})
target_link_libraries(DSBufMgr PUBLIC
    MgrCore
)