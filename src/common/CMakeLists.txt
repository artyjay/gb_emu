#-------------------------------------------------------------------------------
# Author: R.Johnson (artyjay)
# 
# Desc: This file contains the configuration for building the common library.
# 		It exports these targets:
# 
# 		1. common: 			This is an interface
# 		2. gb::common: 		This is a namespaced alias to common
# 
# Copyright 2018
#-------------------------------------------------------------------------------

gb_gather_sources(COMMON_SOURCES "src/common")
gb_add_library(common gb_hw STATIC COMMON_SOURCES CXX)

target_include_directories(common
	PUBLIC		"${PROJECT_SOURCE_DIR}/src/common/public"
	PRIVATE		"${PROJECT_SOURCE_DIR}/src/common/private")

# Alias
add_library(gb::common ALIAS common)