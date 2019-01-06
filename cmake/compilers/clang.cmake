#-------------------------------------------------------------------------------
# Author: R.Johnson (artyjay)
# 
# Desc: This file is used to specify the global compiler settings for all targets
#		when building with GCC.
# 
# Copyright 2018
#-------------------------------------------------------------------------------
set(COMPILER_NAME "clang")
set(CLANG ON)

set(COMPILER_DEFS
	_SCL_SECURE_NO_WARNINGS
	_CRT_SECURE_NO_WARNINGS
	CLANG
	COMPILER=CLANG)

set(COMPILER_FLAGS
	-g3
	-Wall
	-Werror
	-pthread
	-fPIC
	-Wunused
	-Wno-format-security
	-ffunction-sections
	-funwind-tables
	-fstack-protector
	$<$<CONFIG:Deug>:-O0>
	$<$<CONFIG:Release>:-O2>)

set(LINKER_FLAGS "")

set(LINKER_FLAGS_DEBUG
	${LINKER_FLAGS})

set(LINKER_FLAGS_RELEASE
	${LINKER_FLAGS})