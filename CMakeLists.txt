# This file is automatically generated from cmake.toml - DO NOT EDIT
# See https://github.com/build-cpp/cmkr for more information

cmake_minimum_required(VERSION 3.15)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
	message(FATAL_ERROR "In-tree builds are not supported. Run CMake from a separate directory: cmake -B build")
endif()

set(CMKR_ROOT_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
	set(CMKR_ROOT_PROJECT ON)

	# Bootstrap cmkr and automatically regenerate CMakeLists.txt
	include(cmkr.cmake OPTIONAL RESULT_VARIABLE CMKR_INCLUDE_RESULT)
	if(CMKR_INCLUDE_RESULT)
		cmkr()
	endif()

	# Enable folder support
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)

	# Create a configure-time dependency on cmake.toml to improve IDE support
	configure_file(cmake.toml cmake.toml COPYONLY)
endif()

set(ASMJIT_STATIC TRUE)

cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")

project(apitracer
	LANGUAGES
		CXX
		ASM_MASM
)

include(FetchContent)

message(STATUS "Fetching minhook...")
FetchContent_Declare(minhook
	GIT_REPOSITORY
		"https://github.com/TsudaKageyu/minhook.git"
)
FetchContent_MakeAvailable(minhook)

message(STATUS "Fetching asmjit...")
FetchContent_Declare(asmjit
	GIT_REPOSITORY
		"https://github.com/asmjit/asmjit.git"
)
FetchContent_MakeAvailable(asmjit)

message(STATUS "Fetching argparse...")
FetchContent_Declare(argparse
	GIT_REPOSITORY
		"https://github.com/p-ranav/argparse.git"
)
FetchContent_MakeAvailable(argparse)

# Target: ntdll
add_library(ntdll INTERFACE)

target_include_directories(ntdll INTERFACE
	"ntdll/"
)

target_link_directories(ntdll INTERFACE
	"ntdll/"
)

if(CMAKE_SIZEOF_VOID_P EQUAL 4) # x32
	target_link_libraries(ntdll INTERFACE
		ntdll_x86
	)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8) # x64
	target_link_libraries(ntdll INTERFACE
		ntdll_x64
	)
endif()

# Target: tracer
set(tracer_SOURCES
	"tracer/config.cpp"
	"tracer/logger.cpp"
	"tracer/main.cpp"
	"tracer/tracer.cpp"
	"tracer/config.hpp"
	"tracer/logger.hpp"
	"tracer/tracer.hpp"
	"printf/printf.c"
	cmake.toml
)

add_library(tracer SHARED)

target_sources(tracer PRIVATE ${tracer_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${tracer_SOURCES})

target_compile_features(tracer PUBLIC
	cxx_std_23
)

target_include_directories(tracer PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/ntdll/"
	"tracer/"
	"printf/"
)

target_link_libraries(tracer PUBLIC
	ntdll
	minhook
	asmjit
)

target_link_options(tracer PUBLIC
	"/INCREMENTAL:NO"
)

# Target: loader
set(loader_SOURCES
	"loader/main.cpp"
	cmake.toml
)

add_executable(loader)

target_sources(loader PRIVATE ${loader_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${loader_SOURCES})

target_compile_features(loader PRIVATE
	cxx_std_23
)

target_include_directories(loader PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/ntdll/"
	"loader/"
)

target_link_libraries(loader PRIVATE
	ntdll
	argparse
)

target_link_options(loader PRIVATE
	"/INCREMENTAL:NO"
)

get_directory_property(CMKR_VS_STARTUP_PROJECT DIRECTORY ${PROJECT_SOURCE_DIR} DEFINITION VS_STARTUP_PROJECT)
if(NOT CMKR_VS_STARTUP_PROJECT)
	set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT loader)
endif()

