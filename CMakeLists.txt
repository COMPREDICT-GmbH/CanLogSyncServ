# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project("CanLogSyncServ")

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
set(CMAKE_CXX_STANDARD 17)

if (UNIX)
	find_package(PkgConfig)
	pkg_check_modules(PC_ZeroMQ QUIET zmq)
	pkg_check_modules(dbcppp QUIET dbcppp)
	find_path(ZeroMQ_INCLUDE_DIR
			NAMES zmq.hpp
			PATHS ${PC_ZeroMQ_INCLUDE_DIRS}
			)
	find_library(ZeroMQ_LIBRARY
			NAMES zmq
			PATHS ${PC_ZeroMQ_LIBRARY_DIRS}
			)
	find_path(VectorDBC_INCLUDE_DIR
			NAMES dbcppp/DBC_Grammar.h
			PATHS ${PC_dbcppp_INCLUDE_DIRS}
			)
	find_library(Vector_DBC_LIBRARY
			NAMES dbcppp
			PATHS ${PC_dbcppp_LIBRARY_DIRS}
			)
endif ()

find_package(Boost REQUIRED program_options filesystem)
find_package(Protobuf REQUIRED)

# Include sub-projects.
add_subdirectory("src")
