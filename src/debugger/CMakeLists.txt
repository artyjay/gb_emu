#-------------------------------------------------------------------------------
# Author: R.Johnson (artyjay)
# 
# Desc: This file contains the configuration for building the debugger 
#		application. It exports these targets:
# 
# 		1. debugger: This builds an executable.
# 
# Copyright 2018
#-------------------------------------------------------------------------------

gb_prepare_qt()
gb_gather_sources(DBG_SOURCES "src/debugger")
gb_add_executable(debugger gb_debugger DBG_SOURCES CXX)

find_package(Qt5Core CONFIG REQUIRED)
find_package(Qt5Widgets CONFIG REQUIRED)

target_include_directories(debugger
	PUBLIC "${PROJECT_SOURCE_DIR}/src/debugger")

target_link_libraries(debugger
	PRIVATE gb::hw
			Qt5::Core
			Qt5::Widgets)