cmake_minimum_required(VERSION 2.4)
set(CMAKE_SUPPRESS_REGENERATION "1")

project(MayaPartioTools)

set(CMAKE_DEBUG_POSTFIX "_d")

include_directories(${PROJECT_SOURCE_DIR})
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

if(WIN32)
	add_definitions(-DNT_PLUGIN -DGLEW_STATIC)
endif()
if(UNIX)
  add_definitions(-DREQUIRE_IOSTREAM)
  add_compile_options(-fPIC -m64)
  set(PLUGIN_SUFFIX ".so")
  set(CMAKE_SHARED_LIBRARY_SUFFIX ".so")
  set(SUFFIX ".so")
  set(STD_INCLUDES "." ".." "/usr/X11R6/include" "/usr/include" "/usr/include/CC")
  include_directories(${STD_INCLUDES})
  add_definitions(-DLINUX)
endif()

subdirs(
  extern/glew
  extern/partio
  extern/zlib
  )

add_library(MayaPartioTools SHARED
	src/colormap_jet.h
	src/colormap_plasma.h
	src/PluginMain.cpp
	src/PartioEmitter.cpp
	src/PartioEmitter.h
	src/PartioVisualizer.cpp
	src/PartioVisualizer.h
	src/PartioVisualizerDrawOverride.cpp
	src/PartioVisualizerDrawOverride.h
	src/Shader.cpp
	src/Shader.h
)

find_package(Maya)
if (MAYA_FOUND)
  include_directories(${MAYA_INCLUDE_DIR})
  target_link_libraries (MayaPartioTools ${MAYA_LIBRARIES})
endif ()

if(WIN32)
  target_link_libraries(MayaPartioTools opengl32.lib glu32.lib glew partio zlib)
else()
  target_link_libraries(MayaPartioTools GL GLU partio zlib)
endif()
add_dependencies(MayaPartioTools glew partio zlib)

include_directories(${CMAKE_SOURCE_DIR}/extern/glew/include)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)

MAYA_PLUGIN(MayaPartioTools)

add_custom_command(TARGET MayaPartioTools PRE_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy_directory
					${CMAKE_SOURCE_DIR}/scripts $<TARGET_FILE_DIR:MayaPartioTools>/scripts)
					




