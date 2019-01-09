#-------------------------------------------------------------------------------
# Author: R.Johnson (artyjay)
# 
# Desc: This is the entry point for the CMake build system.
# 
# Copyright 2018
#-------------------------------------------------------------------------------
project("gb_emu" C CXX)

cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

#-------------------------------------------------------------------------------
# Project options
#-------------------------------------------------------------------------------
option(GB_ENABLE_FRONTEND_DESKTOP	"Enable building the Game Boy desktop application"	ON)
option(GB_ENABLE_DEBUGGER			"Enable building the Game Boy debugger application"	OFF)
option(GB_ENABLE_TESTS				"Enable building the unit tests"					OFF)

#-------------------------------------------------------------------------------
# CMake configuration
#-------------------------------------------------------------------------------
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: ${CMAKE_CONFIGURATION_TYPES}")
set(CMAKE_SKIP_RPATH TRUE)
set(CMAKE_SKIP_BUILD_RPATH TRUE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
set (CMAKE_CXX_STANDARD 11)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)		# Used by the IDEs for grouping source files.
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/functions.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/conan.cmake)

gb_setup_platform()

if(EMSCRIPTEN)
	set(GB_ENABLE_FRONTEND_DESKTOP	OFF)
	set(GB_ENABLE_DEBUGGER			OFF)
	set(GB_ENABLE_TESTS				OFF)
endif()

#-------------------------------------------------------------------------------
# Conan configuration
#-------------------------------------------------------------------------------
if(NOT EMSCRIPTEN)
conan_cmake_run(CONANFILE conanfile.py
				BASIC_SETUP CMAKE_TARGETS
				BUILD missing)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PLATFORM_BINARIES_PATH})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PLATFORM_BINARIES_PATH})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PLATFORM_BINARIES_PATH})

#-------------------------------------------------------------------------------
# Project configuration
#-------------------------------------------------------------------------------

add_subdirectory(src/hardware)

if(GB_ENABLE_FRONTEND_DESKTOP)
	add_subdirectory(src/desktop)
endif()

if(GB_ENABLE_DEBUGGER)
	add_subdirectory(src/debugger)
endif()

if(GB_ENABLE_TESTS)
	add_subdirectory(src/hardware_tests)
endif()