# Robert Johnson - Copyright 2015

# Setup main project
message(STATUS "Executable ${GBE_DEBUGGER_TARGET}")

rj_prepare_qt()
find_package(Qt5Core)
find_package(Qt5Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# Includes
set(GBE_DEBUGGER_INCLUDES
	"${PROJECT_SOURCE_DIR}/src/debugger"
	"${PROJECT_SOURCE_DIR}/src/hardware"
	${SDL2_INCLUDE_DIR}
)

file(GLOB_RECURSE GBE_DEBUGGER_SOURCES  RELATIVE "${PROJECT_SOURCE_DIR}/build/src/debugger" "${PROJECT_SOURCE_DIR}/src/debugger/*.cpp")  	# Search for .cpp
file(GLOB_RECURSE GBE_DEBUGGER_HEADERS  RELATIVE "${PROJECT_SOURCE_DIR}/build/src/debugger" "${PROJECT_SOURCE_DIR}/src/debugger/*.h")  		# Search for .h
file(GLOB_RECURSE GBE_DEBUGGER_INLINERS RELATIVE "${PROJECT_SOURCE_DIR}/build/src/debugger" "${PROJECT_SOURCE_DIR}/src/debugger/*.inl")  	# Search for .inl
file(GLOB_RECURSE GBE_DEBUGGER_QRC      RELATIVE "${PROJECT_SOURCE_DIR}/build/src/debugger" "${PROJECT_SOURCE_DIR}/src/debugger/*.qrc")  	# Search for .qrc
file(GLOB_RECURSE GBE_DEBUGGER_UI       RELATIVE "${PROJECT_SOURCE_DIR}/build/src/debugger" "${PROJECT_SOURCE_DIR}/src/debugger/*.ui")  	# Search for .ui

# Group source files such that they hierarchically structured in the IDE.
GroupSources(src/debugger)

# Add target as executable.
add_executable(${GBE_DEBUGGER_TARGET} ${GBE_DEBUGGER_SOURCES} ${GBE_DEBUGGER_HEADERS} ${GBE_DEBUGGER_INLINERS} ${GBE_DEBUGGER_QRC} ${GBE_DEBUGGER_UI})

# Setup include/compile/link options
target_compile_options		(${GBE_DEBUGGER_TARGET} PRIVATE ${PLATFORM_COMPILE_FLAGS} ${PLATFORM_HIDDEN_COMPILE_FLAGS})
target_compile_definitions	(${GBE_DEBUGGER_TARGET} PRIVATE ${PLATFORM_COMPILE_DEFS})
target_include_directories	(${GBE_DEBUGGER_TARGET} PRIVATE ${GBE_DEBUGGER_INCLUDES})
target_link_libraries		(${GBE_DEBUGGER_TARGET} ${GBE_HARDWARE_TARGET} ${SDL2_LIBRARY} Qt5::Widgets)

set_target_properties(${GBE_DEBUGGER_TARGET}
	PROPERTIES
	LINKER_LANGUAGE		CXX
	LINK_FLAGS_DEBUG	"${PLATFORM_LINKER_FLAGS_DEBUG}"
	LINK_FLAGS_RELEASE	"${PLATFORM_LINKER_FLAGS_RELEASE}"
)

set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTOUIC OFF)

# Strip
StripBinary(${GBE_DEBUGGER_TARGET})
