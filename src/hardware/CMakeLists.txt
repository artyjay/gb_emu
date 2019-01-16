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

# Compile the Javascript bytecode output into a .js file that ends up in src/web
if(EMSCRIPTEN)
	set(BC_FILE ${PLATFORM_BINARIES_PATH}/${CMAKE_STATIC_LIBRARY_PREFIX}gb_hw${CMAKE_STATIC_LIBRARY_SUFFIX})
	set(JS_FILE ${PROJECT_SOURCE_DIR}/src/web/${CMAKE_STATIC_LIBRARY_PREFIX}gb_hw.js)
	set(HW_FUNCTIONS 
		"_gbhw_create_web"
		"_gbhw_destroy"
		"_gbhw_load_rom_memory"
		"_gbhw_get_screen_web"
		"_gbhw_get_screen_resolution_width"
		"_gbhw_get_screen_resolution_height"
		"_gbhw_step"
		"_gbhw_set_button_state")

	add_custom_command(TARGET hardware POST_BUILD COMMAND python ${PROJECT_SOURCE_DIR}/cmake/scripts/generate_js.py -i ${BC_FILE} -o ${JS_FILE} -f "\"${HW_FUNCTIONS}\"")
endif()

# Alias
add_library(gb::hw ALIAS hardware)

