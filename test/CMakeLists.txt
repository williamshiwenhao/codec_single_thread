set(CMAKE_LEGACY_CYGWIN_WIN32 0)
cmake_minimum_required (VERSION 2.6)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(amr_wb_test amr_wb_test.cpp)
target_link_libraries(amr_wb_test ${COMMON_LIB})

add_executable(codec2_test codec2_test.cpp)
target_link_libraries(codec2_test ${COMMON_LIB})

add_executable(test test.cpp)
target_link_libraries(test ${COMMON_LIB})

add_executable(rtp_test rtp_test.cpp)
target_link_libraries(rtp_test ${COMMON_LIB})

add_executable(codec_test codec_test.cpp)
target_link_libraries(codec_test ${COMMON_LIB})