cmake_minimum_required(VERSION 2.8)
project(wglarbtest)

if(MINGW)
	set(CMAKE_RC_COMPILER_INIT windres)
	enable_language(RC)
	set(CMAKE_RC_COMPILE_OBJECT
		"<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>" )
endif(MINGW)

add_subdirectory(dwm_load)
include_directories(dwm_load)

add_executable(layered
	layered.c
	layered.rc )

target_link_libraries(layered
	wglarb_static
	${OPENGL_gl_LIBRARY} )

add_executable(shared
	shared.c
	shared.rc )

target_link_libraries(shared
	wglarb_static
	${OPENGL_gl_LIBRARY} )

add_executable(customframe
	customframe.c
	customframe.rc )

target_link_libraries(customframe
	wglarb_static
	dwm_load
	${OPENGL_gl_LIBRARY} )
