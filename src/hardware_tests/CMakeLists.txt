#-------------------------------------------------------------------------------
# Author: R.Johnson (artyjay)
# 
# Desc: This file contains the configuration for building the hardware unit tests
#		application. It exports these targets:
# 
# 		1. hardware_tests: This builds an executable.
# 
# Copyright 2018
#-------------------------------------------------------------------------------

gb_gather_sources(HWT_SOURCES "src/hardware_tests")
gb_add_executable(hardware_tests gb_hw_tests HWT_SOURCES CXX)

target_include_directories(hardware_tests
	PUBLIC "${PROJECT_SOURCE_DIR}/src/hardware_tests")

target_link_libraries(hardware_tests
	PRIVATE gb::hw
			CONAN_PKG::gtest)