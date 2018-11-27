#-------------------------------------------------------------------------------
# Author: R.Johnson (artyjay)
# 
# Desc: This file contains the configuration for building the emulator library.
# 		It exports these targets:
# 
# 		1. emulator:		This builds a static library.
# 		2. gb::emulator: 	This is a namespaced alias to emulator
# 
# Copyright 2018
#-------------------------------------------------------------------------------

gb_gather_sources(EMU_SOURCES "src/emulator")
gb_add_library(emulator gb_emulator STATIC EMU_SOURCES CXX)

target_include_directories(emulator
	PUBLIC		"${PROJECT_SOURCE_DIR}/src/emulator/public"
	PRIVATE		"${PROJECT_SOURCE_DIR}/src/emulator/private")

target_compile_definitions(emulator
	PRIVATE		GBEnableGLFW)

target_link_libraries(emulator
	PUBLIC		gb::common
	PRIVATE		gb::hw
				CONAN_PKG::glfw)

# Alias
add_library(gb::emulator ALIAS emulator)

