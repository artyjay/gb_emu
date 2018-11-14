#-------------------------------------------------------------------------------
# Author: R.Johnson (artyjay)
# 
# Desc: This file contains the configuration for building the hardware library.
# 		It exports these targets:
# 
# 		1. hardware: 	This builds a static library.
# 		2. gb::hw: 		This is a namespaced alias to hardware
# 
# Copyright 2018
#-------------------------------------------------------------------------------

gb_gather_sources(HW_SOURCES "src/hardware")
gb_add_library(hardware gb_hw STATIC HW_SOURCES CXX)

target_include_directories(hardware
	PUBLIC "${PROJECT_SOURCE_DIR}/src/hardware")

target_compile_definitions(hardware
	PRIVATE GBExportHW)

# Alias
add_library(gb::hardware ALIAS hardware)