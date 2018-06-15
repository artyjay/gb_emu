#-------------------------------------------------------------------------------
# Author: Robert Johnson (artyjay)
# 
# Desc: This file is used to specify the global compiler settings for all targets
#		when building with GCC.
# 
# Copyright 2018
#-------------------------------------------------------------------------------
set(COMPILER_NAME "gcc")
set(GCC ON)

set(COMPILER_DEFS
	GBInline=inline
	_SCL_SECURE_NO_WARNINGS			# Allow calling any one of the potentially unsafe methods in the Standard C++ Library
	_CRT_SECURE_NO_WARNINGS			# Disable CRT deprecation warnings
	GCC)

set(COMPILER_FLAGS
	-g3
	-Wall
	-Werror
	-pthread
	-fPIC
	-Wunused
	-ffunction-sections
	-funwind-tables
	-fstack-protector
	$<$<CONFIG:Debug>:-O0>
	$<$<CONFIG:Release>:-O2>)

set(LINKER_FLAGS
	-Wl
	--no-as-needed
	-pthread
	--exclude-libs,ALL)

set(LINKER_FLAGS_DEBUG
	${PLATFORM_LINKER_FLAGS})

set(LINKER_FLAGS_RELEASE
	${PLATFORM_LINKER_FLAGS})