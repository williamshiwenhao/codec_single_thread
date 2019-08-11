cmake_minimum_required (VERSION 2.6)
set(CMAKE_LEGACY_CYGWIN_WIN32 0)
project (codec_changer)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(CMAKE_CXX_FLAGS "-pipe -g -std=c++11 -W -Wall -Wno-unused-parameter -Wno-deprecated-declarations -Wno-reorder -fPIC")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
#set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_BUILD_TYPE "RelWithDebInfo")
set(CMAKE_BUILD_TYPE "Debug")
ADD_DEFINITIONS(-DLINUX -DPTHREAD)
message(INFO "CMAKE BUILD TYPE:" ${CMAKE_BUILD_TYPE})
set(PROJECT_DIR  ${CMAKE_CURRENT_SOURCE_DIR})



file(GLOB_RECURSE MAIN_SOURCES "src/*.cpp")


set(SERVICE_SOURCES ${MAIN_SOURCES})
set(SERVICE_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/include/")

set(CODEC_MODULE "${PROJECT_DIR}/lib")
set(GLOB_RECURSE LIBCODEC2 "${CODEC_MODULE}/libcodec2.a")
set(GLOB_RECURSE LIBAMRWB "${CODEC_MODULE}/libamr_wb.a")

set(GLOB_RECURSE LIBSAMPLERATE "/usr/local/lib/libsamplerate.a")






include_directories(
    ${SERVICE_INCLUDE}
    )

add_executable(server ${SERVICE_SOURCES})

target_link_libraries(server
    ${LIBAMRWB}
    ${LIBCODEC2}
    ${LIBSAMPLERATE}
    pthread
    )