cmake_minimum_required(VERSION 3.12)
project(AudioServer)

set(AUDIOSERVER_VERSION_MAJOR 0)
set(AUDIOSERVER_VERSION_MINOR 1)
set(AUDIOSERVER_VERSION_REVISION 0)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR OR EXISTS "${CMAKE_BINARY_DIR}/CMakeLists.txt")
  message(FATAL_ERROR "In-source builds are not allowed.")
endif()

if(NOT WIN32)
  message(FATAL "This project targets Windows")
endif()

# windows.winmd search path
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -AI\"C:/Program Files (x86)/Windows Kits/10/UnionMetadata/10.0.18362.0\"" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -AI\"C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/IDE/VC/vcpackages\"" )

find_package(cpprestsdk CONFIG REQUIRED)

add_subdirectory(lib/IAudioServer)
add_subdirectory(lib/cppaudio)
add_subdirectory(lib/cpplogger)

set(CMAKE_CXX_STANDARD 17)
add_compile_options("/ZW")

file(GLOB SOURCES "./src/*")
add_library(AudioServer SHARED ${SOURCES})
target_link_libraries(AudioServer IAudioServer cpplogger cppaudio avrt.lib mmdevapi.lib cpprestsdk::cpprest cpprestsdk::cpprestsdk_zlib_internal cpprestsdk::cpprestsdk_boost_internal cpprestsdk::cpprestsdk_brotli_internal OleAut32.lib shlwapi.lib)
