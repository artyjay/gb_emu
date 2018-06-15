# Robert Johnson - Copyright 2017

# Setup main project
message(STATUS "Executable ${GBE_HARDWARE_TESTS_TARGET}")

# Paths & files
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
set(GBE_HARDWARE_TESTS_INCLUDES
	"${PROJECT_SOURCE_DIR}/src/debugger"
	"${PROJECT_SOURCE_DIR}/src/hardware"
	"${PROJECT_SOURCE_DIR}/libs/gtest/include"
)

file(GLOB_RECURSE GBE_HARDWARE_TESTS_SOURCES
	"${PROJECT_SOURCE_DIR}/src/hardware_tests/*.cpp"
	"${PROJECT_SOURCE_DIR}/src/hardware_tests/*.h"
	"${PROJECT_SOURCE_DIR}/src/hardware_tests/*.inl"
)

# Group source files such that they hierarchically structured in the IDE.
GroupSources(src/hardware_tests src/hardware_tests)

# Add target as executable.
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
add_executable(${GBE_HARDWARE_TESTS_TARGET} ${GBE_HARDWARE_TESTS_SOURCES})

# Setup include/compile/link options
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
target_compile_options		(${GBE_HARDWARE_TESTS_TARGET} PRIVATE ${PLATFORM_COMPILE_FLAGS})
target_compile_definitions	(${GBE_HARDWARE_TESTS_TARGET} PRIVATE ${PLATFORM_COMPILE_DEFS})
target_include_directories	(${GBE_HARDWARE_TESTS_TARGET} PRIVATE ${GBE_HARDWARE_TESTS_INCLUDES})
target_link_libraries		(${GBE_HARDWARE_TESTS_TARGET} ${GBE_HARDWARE_TARGET} gtest_main)

set_target_properties(${GBE_HARDWARE_TESTS_TARGET}
	PROPERTIES
	LINKER_LANGUAGE		CXX
	LINK_FLAGS			"${PLATFORM_LINKER_FLAGS}"
)