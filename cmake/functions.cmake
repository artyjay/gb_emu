#-------------------------------------------------------------------------------
# Author: R.Johnson (artyjay)
# 
# Desc: This file contains several helper functions utilised by the rest of the
#		build system.
# 
# Copyright 2018
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Helper for generating source code hierarchy in the IDE by grouping based upon
# folder hierarchy
#-------------------------------------------------------------------------------
macro(gb_group_sources SOURCE_DIR ROOT_DIR)
	file(GLOB CHILDREN RELATIVE ${PROJECT_SOURCE_DIR}/${SOURCE_DIR}	${PROJECT_SOURCE_DIR}/${SOURCE_DIR}/*)

	if(${ROOT_DIR} STREQUAL ${SOURCE_DIR})
		set(GROUP_NAME " ")
	else()
		string(REPLACE "${RELATIVE_DIR}/" "" RELATIVE_SOURCE_DIR ${SOURCE_DIR})
		string(REPLACE "/" "\\" GROUP_NAME ${RELATIVE_SOURCE_DIR})
	endif()

	foreach(CHILD ${CHILDREN})
		if(IS_DIRECTORY ${PROJECT_SOURCE_DIR}/${SOURCE_DIR}/${CHILD})
			gb_group_sources(${SOURCE_DIR}/${CHILD} ${ROOT_DIR})
		else()
			source_group(${GROUP_NAME} FILES ${PROJECT_SOURCE_DIR}/${SOURCE_DIR}/${CHILD})
		endif()
	endforeach()
endmacro()

#-------------------------------------------------------------------------------
# Detects the chosen platform architecture for selecting the correct libraries.
#-------------------------------------------------------------------------------
macro(gb_detect_architecture)
	set(ARCH_NAME "x86")

	if(ANDROID)
		if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL i686)
			set(ARCH_NAME "x86")
		elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL x86_64)
			set(ARCH_NAME "x64")
		else()
			set(ARCH_NAME "${CMAKE_SYSTEM_PROCESSOR}")
		endif()
	elseif(EMSCRIPTEN)
		set(ARCH_NAME "web")
	else()
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(ARCH_NAME "x64")
		else()
			set(ARCH_NAME "x86")
		endif()
	endif()
endmacro()

#-------------------------------------------------------------------------------
# Detects the chosen os, this provides some override behaviour for when the OS
# is agnostic based upon toolchain and arch.
#-------------------------------------------------------------------------------
macro(gb_detect_os)
	if(EMSCRIPTEN)
		set(OS_NAME "generic")
	elseif(WIN32)
		set(OS_NAME "windows")
	elseif(ANDROID)
		set(OS_NAME "android")
	elseif()
		set(OS_NAME "linux")
	else()
		message(FATAL_ERROR "Failed to detect OS")
	endif()
endmacro()

#-------------------------------------------------------------------------------
# Sets up the compiler based upon the detected local architecture/OS.
#-------------------------------------------------------------------------------
macro(gb_detect_compiler)
	if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
		include(${PROJECT_SOURCE_DIR}/cmake/compilers/msvc.cmake)
	elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
		include(${PROJECT_SOURCE_DIR}/cmake/compilers/gcc.cmake)
	elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
		message(FATAL_ERROR "Clang is not a currently supported compiler")
	elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL Intel)
		message(FATAL_ERROR "ICC is not a currently supported compiler")
	else()
		message(FATAL_ERROR "Unrecognised compiler")
	endif()
endmacro()

#-------------------------------------------------------------------------------
# Setups the platfrom variables needed by project.
#-------------------------------------------------------------------------------
macro(gb_setup_platform)
	gb_detect_architecture()
	gb_detect_os()
	gb_detect_compiler()

	set(PLATFORM_TRIPLET ${ARCH_NAME}-${OS_NAME}-${COMPILER_NAME})
	set(PLATFORM_BINARIES_PATH ${PROJECT_SOURCE_DIR}/build/${PLATFORM_TRIPLET})

	message(STATUS "Target platform: ${PLATFORM_TRIPLET}")
	message(STATUS "Binaries path: ${PLATFORM_BINARIES_PATH}")
endmacro()

#-------------------------------------------------------------------------------
# Helper for setting up the QT dependency
#-------------------------------------------------------------------------------
macro(gb_prepare_qt)
	set(CMAKE_INCLUDE_CURRENT_DIR ON)
	set(CMAKE_AUTOMOC ON)
	set(CMAKE_AUTOUIC ON)

	find_package(Qt5Core)
	find_package(Qt5Widgets)
endmacro()

#-------------------------------------------------------------------------------
# Helper that gathers all the common source files in a given folder tree and
# outputs them on the SOURCES variable in the callers scope.
#-------------------------------------------------------------------------------
function(gb_gather_sources SOURCES PATH)
	set(ABSPATH "${PROJECT_SOURCE_DIR}/${PATH}")

	file(GLOB_RECURSE FOUND_FILES 
			"${ABSPATH}/*.cpp"
			"${ABSPATH}/*.c"
			"${ABSPATH}/*.cc"
			"${ABSPATH}/*.h"
			"${ABSPATH}/*.hpp"
			"${ABSPATH}/*.inl"
			"${ABSPATH}/*.qrc"
			"${ABSPATH}/*.ui")

	if(WIN32)
		file(GLOB_RECURSE FOUND_FILES_WIN32 "${ABSPATH}/*.rc")
	endif()

	gb_group_sources(${PATH} ${PATH})

	set(${SOURCES} ${FOUND_FILES} ${FOUND_FILES_WIN32} PARENT_SCOPE)
endfunction()

#-------------------------------------------------------------------------------
# Helper macro used for setting up common properties of a target.
#-------------------------------------------------------------------------------
macro(gb_common_props NAME BINARY LANG)
	target_compile_options (${NAME} PRIVATE ${COMPILER_FLAGS})
	target_compile_definitions(${NAME} PRIVATE ${COMPILER_DEFS})
	set_target_properties(${NAME}
		PROPERTIES
		OUTPUT_NAME					${BINARY}
		LINKER_LANGUAGE				${LANG}
		LINK_FLAGS_DEBUG			"${LINKER_FLAGS_DEBUG}"
		LINK_FLAGS_RELEASE			"${LINKER_FLAGS_RELEASE}")
endmacro()

#-------------------------------------------------------------------------------
# Helper that adds a library to the system and sets up common properties.
#-------------------------------------------------------------------------------
function(gb_add_library NAME BINARY TYPE SOURCES LANG)
	message(STATUS "${NAME} (${BINARY})")

	set(SOURCE_FILES "${${SOURCES}}")	## Evaluate list of source files
	add_library(${NAME} ${TYPE} ${SOURCE_FILES})
	gb_common_props(${NAME} ${BINARY} ${LANG})
endfunction()

#-------------------------------------------------------------------------------
# Helper that adds an executable to the system and sets up common properties.
#-------------------------------------------------------------------------------
function(gb_add_executable NAME BINARY SOURCES LANG)
	message(STATUS "${NAME} (${BINARY})")

	set(SOURCE_FILES "${${SOURCES}}")	## Evaluate list of source files
	add_executable(${NAME} ${SOURCE_FILES})
	gb_common_props(${NAME} ${BINARY} ${LANG})
endfunction()