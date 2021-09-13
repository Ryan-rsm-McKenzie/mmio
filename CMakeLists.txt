cmake_minimum_required(VERSION 3.21)

project(
	mmio
	VERSION 1.1.0
	LANGUAGES CXX
)

if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
	message(FATAL_ERROR "in-source builds are not allowed")
endif()

option(MMIO_BUILD_SRC "whether we should build the library itself" ON)
if(MMIO_BUILD_SRC)
	add_subdirectory(src)
endif()

include(CTest)
if(BUILD_TESTING)
	find_package(Catch2 REQUIRED CONFIG)
	include(Catch)
	add_subdirectory(tests)
endif()
