#-------------------------------------------------------------------------------
# Author: Robert Johnson (artyjay)
# 
# Desc: This file contains the configuration for building the desktop frontend
#		application. It exports these targets:
# 
# 		1. desktop: 		This builds an executable.
# 
# Copyright 2018
#-------------------------------------------------------------------------------

rj_gather_sources(DT_SOURCES "src/desktop")
rj_add_executable(desktop gb_desktop DT_SOURCES CXX)

target_compile_definitions(desktop
	PRIVATE SDL_MAIN_HANDLED)

target_link_libraries(desktop
	gb::hardware
	CONAN_PKG::sdl2)