#-------------------------------------------------------------------------------
# Author: R.Johnson (artyjay)
# 
# Desc: This file contains the configuration for building the desktop frontend
#		application. It exports these targets:
# 
# 		1. desktop: This builds an executable.
# 
# Copyright 2018
#-------------------------------------------------------------------------------

gb_gather_sources(DT_SOURCES "src/desktop")
gb_add_executable(desktop gb_desktop DT_SOURCES CXX)

target_compile_definitions(desktop
	PRIVATE SDL_MAIN_HANDLED)

target_link_libraries(desktop
	PUBLIC		gb::hw
	PRIVATE		CONAN_PKG::sdl2)