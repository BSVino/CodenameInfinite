include_directories (
	${PROJECT_SOURCE_DIR}/../ext-deps/EASTL/include
	${CMAKE_SOURCE_DIR}/
	${CMAKE_SOURCE_DIR}/common
	${CMAKE_SOURCE_DIR}/common/math
	${CMAKE_SOURCE_DIR}/tinker
	${CMAKE_SOURCE_DIR}/sp/src/
)

set (SPTEST_SOURCES
	main.cpp
	scalefloat.cpp
	scalematrix.cpp
	${CMAKE_SOURCE_DIR}/sp/src/sp_common.cpp
)

add_executable (sptests ${SPTEST_SOURCES})
target_link_libraries (sptests lwbase tinker)
