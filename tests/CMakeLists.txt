set(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

set(SOURCE_DIR "${ROOT_DIR}/tests")
set(SOURCE_FILES
	"${SOURCE_DIR}/mmio/mmio.test.cpp"
)

source_group(TREE "${SOURCE_DIR}" PREFIX "src" FILES ${SOURCE_FILES})

add_executable(
	tests
	${HEADER_FILES}
	${SOURCE_FILES}
)
catch_discover_tests(tests)

target_include_directories(
	tests
	PRIVATE
		"${SOURCE_DIR}"
)

target_link_libraries(
	tests
	PRIVATE
		Catch2::Catch2WithMain
		mmio::mmio
)
