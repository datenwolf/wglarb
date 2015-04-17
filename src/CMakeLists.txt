cmake_minimum_required(VERSION 2.8)
project(libwglarb)

include(GenerateExportHeader)

# GCC intrinsics support and C++ standard library should be linked statically
# so that no external dependencies are created.
if(NOT MSVC) # TODO: this should test for GCC and not "not MSVC" compiler
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc -std=c99")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
	set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS} -static-libgcc -s")
	set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS} -static-libgcc -static-libstdc++ -s")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++ -s ")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--output-def,wglarb${DLL_OUTPUT_SUFFIX}.def,--out-implib,wglarb.dll.a")
endif()

set(OPENGL_API_LIBRARY opengl32)

set(LIBWGLARB_INCLUDE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE STRING "wglarb source directory" FORCE)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR} )
set(LIBWGLARB_SRC_FILES
	wglarb.c )

add_library(wglarb_static STATIC 
	${LIBWGLARB_SRC_FILES} )

add_library(wglarb_shared SHARED 
	${LIBWGLARB_SRC_FILES} )

target_link_libraries(
	wglarb_shared
	${OPENGL_API_LIBRARY} )

set_target_properties(wglarb_shared PROPERTIES DEFINE_SYMBOL "wglarb_EXPORTS")
set_target_properties(wglarb_shared PROPERTIES DEFINE_SYMBOL "wglarb_EXPORTS")

set_target_properties(wglarb_shared PROPERTIES OUTPUT_NAME "wglarb")
set_target_properties(wglarb_static PROPERTIES OUTPUT_NAME "wglarb")

set_target_properties(wglarb_shared PROPERTIES PREFIX "${DLL_OUTPUT_PREFIX}")
set_target_properties(wglarb_shared PROPERTIES SUFFIX "${DLL_OUTPUT_SUFFIX}.dll")
