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
	PUBLIC		"${PROJECT_SOURCE_DIR}/src/hardware/public"
	PRIVATE		"${PROJECT_SOURCE_DIR}/src/hardware/private")

if(GB_ENABLE_DEBUGGER)
	target_include_directories(hardware
		PUBLIC		"${PROJECT_SOURCE_DIR}/src/hardware/private"
					"${PROJECT_SOURCE_DIR}/src/hardware/debug")
	target_compile_definitions(hardware
		PRIVATE		HWEnableDebug)
else()
	target_include_directories(hardware
		PRIVATE		"${PROJECT_SOURCE_DIR}/src/hardware/debug")
endif()

# Alias
add_library(gb::hw ALIAS hardware)

