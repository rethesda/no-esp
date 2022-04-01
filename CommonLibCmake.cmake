cmake_minimum_required(VERSION 3.20)
set(NAME "no-esp")
set(VERSION 1.0.0)
set(VR_VERSION 1)
set(AE_VERSION 1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# ---- Options ----

option(COPY_BUILD "Copy the build output to the Skyrim directory." TRUE)
option(BUILD_SKYRIMVR "Build for Skyrim VR" OFF)
option(BUILD_SKYRIMAE "Build for Skyrim AE" OFF)

# ---- Cache build vars ----

macro(set_from_environment VARIABLE)
	if (NOT DEFINED ${VARIABLE} AND DEFINED ENV{${VARIABLE}})
		set(${VARIABLE} $ENV{${VARIABLE}})
	endif ()
endmacro()

macro(find_commonlib_path)
	if (CommonLibName AND NOT ${CommonLibName} STREQUAL "")
		# Check extern
		find_path(CommonLibPath
		include/REL/Relocation.h
		PATHS CommonLib/AE)
		if (${CommonLibPath} STREQUAL "CommonLibPath-NOTFOUND")
			#Check path
			set_from_environment(${CommonLibName}Path)
			set(CommonLibPath ${${CommonLibName}Path})
		endif()
	endif()
endmacro()

set_from_environment(VCPKG_ROOT)
if(BUILD_SKYRIMAE)
	add_compile_definitions(SKYRIM_AE)
	set(CommonLibName "CommonLibSSE")
	set_from_environment(SkyrimAEPath)
	set(SkyrimPath ${SkyrimAEPath})
	set(SkyrimVersion "Skyrim AE")
	set(VERSION ${VERSION}.${AE_VERSION})
elseif(BUILD_SKYRIMVR)
	add_compile_definitions(SKYRIMVR)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
	set(CommonLibName "CommonLibVR")
	set_from_environment(SkyrimVRPath)
	set(SkyrimPath ${SkyrimVRPath})
	set(SkyrimVersion "Skyrim VR")
	set(VERSION ${VERSION}.${VR_VERSION})
else()
	set(CommonLibName "CommonLibSSE")
	set_from_environment(Skyrim64Path)
	set(SkyrimPath ${Skyrim64Path})
	set(SkyrimVersion "Skyrim SSE")
endif()
find_commonlib_path()
message(
	STATUS
	"Building ${NAME} ${VERSION} for ${SkyrimVersion} at ${SkyrimPath} with ${CommonLibName} at ${CommonLibPath}."
)


if (DEFINED VCPKG_ROOT)
	set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
	set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "")
else ()
	message(
		WARNING
		"Variable VCPKG_ROOT is not set. Continuing without vcpkg."
	)
endif ()

set(Boost_USE_STATIC_RUNTIME OFF CACHE BOOL "")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")

# ---- Project ----

project(no-esp VERSION 0.0.1 LANGUAGES CXX)

#configure_file(
#	${CMAKE_CURRENT_SOURCE_DIR}/cmake/Version.h.in
#	${CMAKE_CURRENT_BINARY_DIR}/include/Version.h
#	@ONLY
#)
#
#configure_file(
#	${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.rc.in
#	${CMAKE_CURRENT_BINARY_DIR}/version.rc
#	@ONLY
#)

# ---- Include guards ----

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
	message(
		FATAL_ERROR
			"In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
	)
endif()

# ---- Globals ----

add_compile_definitions(
	SKSE_SUPPORT_XBYAK
)
if (BUILD_SKYRIMAE)
	add_compile_definitions(
		SKYRIM_SUPPORT_AE
	)
endif()

if (MSVC)
	add_compile_definitions(
		_UNICODE
	)
	if (NOT ${CMAKE_GENERATOR} STREQUAL "Ninja")
		add_compile_options(
			/MP	# Build with Multiple Processes
		)
	endif ()
endif ()

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)

set(Boost_USE_STATIC_LIBS ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")

# ---- Dependencies ----

set(CommonLibSSEPath ${CMAKE_CURRENT_SOURCE_DIR}/CommonLib/AE)

if (DEFINED CommonLibSSEPath AND NOT ${CommonLibSSEPath} STREQUAL "" AND IS_DIRECTORY ${CommonLibSSEPath})
	add_subdirectory(${CommonLibSSEPath} CommonLibSSE)
else ()
	message(
		FATAL_ERROR
		"Variable CommonLibSSEPath is not set."
	)
endif ()

find_package(spdlog REQUIRED CONFIG)
find_package(xbyak REQUIRED CONFIG)

find_path(SIMPLEINI_INCLUDE_DIRS "ConvertUTF.c")

# ---- Add source files ----

file(GLOB_RECURSE sources src/*.cpp include/*.h)

#source_group(
#	TREE
#		${CMAKE_CURRENT_SOURCE_DIR}
#	FILES
#		${headers}
#		${sources}
#)

#source_group(
#	TREE
#		${CMAKE_CURRENT_BINARY_DIR}
#	FILES
#		${CMAKE_CURRENT_BINARY_DIR}/include/Version.h
#)

# ---- Create DLL ----

add_library(
	${PROJECT_NAME}
	SHARED
	${headers}
	${sources}
)
target_compile_features("${PROJECT_NAME}" PRIVATE cxx_std_23)
target_include_directories(no-esp PRIVATE include)
#	${CMAKE_CURRENT_BINARY_DIR}/include/Version.h
#	${CMAKE_CURRENT_BINARY_DIR}/version.rc
#	.clang-format
# "src/aowMenu.cpp" "src/Events.cpp"  "src/Settings.h")

target_compile_features(
	${PROJECT_NAME}
	PRIVATE
		cxx_std_20
)

target_compile_definitions(
	${PROJECT_NAME}
	PRIVATE
		_UNICODE
)

target_include_directories(
	${PROJECT_NAME}
	PRIVATE
		${CMAKE_CURRENT_BINARY_DIR}/include
		${CMAKE_CURRENT_SOURCE_DIR}/src
		${SIMPLEINI_INCLUDE_DIRS}
)

target_link_libraries(
	${PROJECT_NAME}
	PRIVATE
		CommonLibSSE::CommonLibSSE
		spdlog::spdlog
		xbyak::xbyak
)

#target_precompile_headers(
#	${PROJECT_NAME}
#	PRIVATE
#		src/PCH.h
#)

if (MSVC)
	target_compile_options(
		${PROJECT_NAME}
		PRIVATE
			/sdl	# Enable Additional Security Checks
			/utf-8	# Set Source and Executable character sets to UTF-8
			/Zi	# Debug Information Format

			/permissive-	# Standards conformance

			/Zc:alignedNew	# C++17 over-aligned allocation
			/Zc:auto	# Deduce Variable Type
			/Zc:char8_t
			/Zc:__cplusplus	# Enable updated __cplusplus macro
			/Zc:externC
			/Zc:externConstexpr	# Enable extern constexpr variables
			/Zc:forScope	# Force Conformance in for Loop Scope
			/Zc:hiddenFriend
			/Zc:implicitNoexcept	# Implicit Exception Specifiers
			/Zc:lambda
			/Zc:noexceptTypes	# C++17 noexcept rules
			/Zc:preprocessor	# Enable preprocessor conformance mode
			/Zc:referenceBinding	# Enforce reference binding rules
			/Zc:rvalueCast	# Enforce type conversion rules
			/Zc:sizedDealloc	# Enable Global Sized Deallocation Functions
			/Zc:strictStrings	# Disable string literal type conversion
			/Zc:ternary	# Enforce conditional operator rules
			/Zc:threadSafeInit	# Thread-safe Local Static Initialization
			/Zc:tlsGuards
			/Zc:trigraphs	# Trigraphs Substitution
			/Zc:wchar_t	# wchar_t Is Native Type

			/external:anglebrackets
			/external:W0

			/W4	# Warning level

			"$<$<CONFIG:DEBUG>:>"
			"$<$<CONFIG:RELEASE>:/Zc:inline;/JMC-;/Ob3>"
	)

	target_link_options(
		${PROJECT_NAME}
		PRIVATE
			/WX	# Treat Linker Warnings as Errors

			"$<$<CONFIG:DEBUG>:/INCREMENTAL;/OPT:NOREF;/OPT:NOICF>"
			"$<$<CONFIG:RELEASE>:/INCREMENTAL:NO;/OPT:REF;/OPT:ICF;/DEBUG:FULL>"
	)

endif ()

# ---- Post build ----

set(Skyrim64Path "C:/Users/mrowr/Dropbox/Skyrim/Mod Authoring/Mods/No ESP - AE/")

if (COPY_BUILD)
	if (DEFINED Skyrim64Path)
		add_custom_command(
			TARGET ${PROJECT_NAME}
			POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${Skyrim64Path}/SKSE/Plugins/
			COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PDB_FILE:${PROJECT_NAME}> ${Skyrim64Path}/SKSE/Plugins/
		)
	else ()
		message(
			WARNING
			"Variable Skyrim64Path is not defined. Skipping post-build copy command."
		)
	endif ()