#-------------------------------------------------------------------------------
# Author: R.Johnson (artyjay)
# 
# Desc: This file is used to specify the global compiler settings for all targets
#		when building with MSVC.
# 
# Copyright 2018
#-------------------------------------------------------------------------------
set(COMPILER_NAME "msvc")
set(MSVC ON)

set(COMPILER_DEFS
	GBInline=__forceinline
	_SCL_SECURE_NO_WARNINGS			# Allow calling any one of the potentially unsafe methods in the Standard C++ Library
	_CRT_SECURE_NO_WARNINGS			# Disable CRT deprecation warnings
	MSVC)

set(COMPILER_FLAGS
	#/MT$<$<CONFIG:Debug>:d>		# Use multi-threaded runtime, choosing either /MT or /MTd depending on the config.
	/nologo
	/Zc:wchar_t
	/Zc:forScope
	/GF
	/GS
	/Zi
	/MP
	/W3
	/WX
	/Oi
	/GR-)

set(LINKER_FLAGS
	/NOLOGO
	/INCREMENTAL:NO
	/NXCOMPAT
	/DYNAMICBASE:NO)

set(LINKER_FLAGS_DEBUG
	${PLATFORM_LINKER_FLAGS}
	/DEBUG)

set(LINKER_FLAGS_RELEASE
	${PLATFORM_LINKER_FLAGS}
	/OPT:REF
	/OPT:ICF
	/LTCG
	/DELAY:UNLOAD)

string(REPLACE ";" " " LINKER_FLAGS_DEBUG "${LINKER_FLAGS_DEBUG}")
string(REPLACE ";" " " LINKER_FLAGS_RELEASE "${LINKER_FLAGS_RELEASE}")