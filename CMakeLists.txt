# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project("CanLogSyncServ")

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
set(CMAKE_CXX_STANDARD 17)

if (UNIX)
	find_package(PkgConfig)
	pkg_check_modules(PC_ZeroMQ QUIET zmq)
	pkg_check_modules(PC_VectorDBC QUIET Vector_DBC)
	find_path(ZeroMQ_INCLUDE_DIR
			NAMES zmq.hpp
			PATHS ${PC_ZeroMQ_INCLUDE_DIRS}
			)
	find_library(ZeroMQ_LIBRARY
			NAMES zmq
			PATHS ${PC_ZeroMQ_LIBRARY_DIRS}
			)
	find_path(VectorDBC_INCLUDE_DIR
			NAMES Vector/DBC.h
			PATHS ${PC_VectorDBC_INCLUDE_DIRS}
			)
	find_library(Vector_DBC_LIBRARY
			NAMES Vector_DBC
			PATHS ${PC_VectorDBC_LIBRARY_DIRS}
			)
endif ()

find_package(Boost REQUIRED program_options)

# Include sub-projects.
add_subdirectory("src")
