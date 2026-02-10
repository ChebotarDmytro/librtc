cmake_minimum_required(VERSION 3.20)

# Set compilers before project() call
set(CMAKE_CXX_COMPILER /usr/bin/clang++-21)
set(CMAKE_C_COMPILER /usr/bin/clang-21)

# Force use of LLVM linker (lld) instead of system ld
# This is critical to avoid architecture mismatch errors
set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld")
set(CMAKE_SHARED_LINKER_FLAGS "-fuse-ld=lld")

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(librtc LANGUAGES CXX)

# Validate build type
set(ALLOWED_BUILD_TYPES "Debug" "RelWithDebInfo")
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

list(FIND ALLOWED_BUILD_TYPES ${CMAKE_BUILD_TYPE} INDEX)
if(INDEX EQUAL -1)
    message(FATAL_ERROR
            "Invalid build type: ${CMAKE_BUILD_TYPE}. "
            "Only Debug and RelWithDebInfo are supported.")
endif()

# Find dependencies
find_package(webrtc REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

# Header files
set(LIBRTC_HEADERS
    include/librtc/data_channel.hpp
    include/librtc/peer_connection.hpp
    include/librtc/errors/data_channel_error.hpp
    include/librtc/errors/peer_connection_error.hpp
    include/librtc/utils/event.hpp
    include/librtc/utils/expected.hpp
    include/librtc/utils/async_bridge.hpp
    src/impl/data_channel_impl.hpp
    src/impl/peer_connection_impl.hpp
)

# Source files
set(LIBRTC_SOURCES
    src/peer_connection.cpp
    src/impl/data_channel_impl.cpp
    src/impl/peer_connection_impl.cpp
)

# Library build
add_library(librtc STATIC ${LIBRTC_HEADERS} ${LIBRTC_SOURCES})
set_target_properties(librtc PROPERTIES OUTPUT_NAME rtc)
target_include_directories(librtc PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(librtc PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(librtc PUBLIC 
    webrtc::webrtc 
    Boost::headers
    Boost::system
)

# Compiler flags matching WebRTC
target_compile_options(librtc PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    -Wno-unused-variable
    -Wno-nullability-completeness
    -Wno-nullability-extension
    -Wno-deprecated-builtins
    -fno-rtti                           # WebRTC disables RTTI
    -fvisibility=hidden
    -fvisibility-inlines-hidden
)

# Build type specific flags for the library
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(librtc PRIVATE _DEBUG)
    target_compile_options(librtc PRIVATE -O0 -g3)
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    target_compile_definitions(librtc PRIVATE NDEBUG)
    target_compile_options(librtc PRIVATE -O2 -g1)
endif()

# Test executable
add_executable(hello_world examples/hello_world_test.cpp)
target_link_libraries(hello_world PRIVATE librtc)

target_compile_options(hello_world PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -fno-rtti
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(hello_world PRIVATE _DEBUG)
    target_compile_options(hello_world PRIVATE -O0 -g3)
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    target_compile_definitions(hello_world PRIVATE NDEBUG)
    target_compile_options(hello_world PRIVATE -O2 -g1)
endif()


# Formatting target
find_program(CLANG_FORMAT_EXE clang-format)
if(CLANG_FORMAT_EXE)
    # Use absolute paths for all files to ensure they are found regardless of the build directory
    set(FORMAT_FILES "")
    foreach(file ${LIBRTC_HEADERS} ${LIBRTC_SOURCES} test/hello_world_test.cpp)
        list(APPEND FORMAT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${file}")
    endforeach()

    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i ${FORMAT_FILES}
        COMMENT "Formatting source files..."
    )
endif()

# Print build configuration summary
message(STATUS "")
message(STATUS "=== librtc Build Configuration ===")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Linker: lld")
message(STATUS "WebRTC package: FOUND")
message(STATUS "Boost version: ${Boost_VERSION}")
message(STATUS "===================================")
message(STATUS "")
