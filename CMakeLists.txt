set(CMAKE_LEGACY_CYGWIN_WIN32 0)
cmake_minimum_required (VERSION 2.6)
project (codec_changer)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(CMAKE_CXX_FLAGS "-pipe -g -std=c++11 -W -Wall -Wno-unused-parameter -Wno-deprecated-declarations -Wno-reorder -fPIC")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_BUILD_TYPE "Debug")
ADD_DEFINITIONS(-DLINUX -DPTHREAD -D_BSD_SOURCE)
set(PROJECT_DIR  ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})



file(GLOB_RECURSE MAIN_SOURCES "src/*.cpp")
list(REMOVE_ITEM MAIN_SOURCES "${PROJECT_DIR}/src/main.cpp")


set(SERVICE_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/include/")

link_directories(
    "${PROJECT_DIR}/lib"
    "/usr/local/lib"
)

include_directories(
    src/
    ${SERVICE_INCLUDE}
    )


set(SRC_LIB codec_common_lib)
add_library(${SRC_LIB} STATIC ${MAIN_SOURCES})
set(COMMON_LIB
    ${SRC_LIB}
    codec2
    amr_wb
    samplerate
    pthread
)


add_executable(${PROJECT_NAME}  "src/main.cpp")
target_link_libraries(${PROJECT_NAME} ${COMMON_LIB})
add_subdirectory(test)