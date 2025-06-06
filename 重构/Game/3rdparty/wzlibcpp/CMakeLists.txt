cmake_minimum_required(VERSION 3.17)
project(wzlibcpp)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(zlib)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/zlib")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/mio/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/wz")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

file(GLOB SOURCE_FILES src/*.cpp)
file(GLOB AES_SOURCE_FILES AES/AES.cpp)

add_library(wzlib ${SOURCE_FILES} ${AES_SOURCE_FILES})
target_link_libraries(wzlib zlibstatic)

if(WIN32)
        project(wzlibtest)

        add_executable(wzlibtest main/main.cpp ${SOURCE_FILES})
        target_link_libraries(
                wzlibtest
                PRIVATE
                wzlib
        )
endif()