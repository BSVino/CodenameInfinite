include_directories (
	${PROJECT_SOURCE_DIR}/ext-deps/EASTL/include
	${CMAKE_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/common
	${CMAKE_SOURCE_DIR}/common/math
	${CMAKE_SOURCE_DIR}/tengine
	${CMAKE_SOURCE_DIR}/sp/src/
)

if (WIN32)
	include_directories(${CMAKE_SOURCE_DIR}/ext-deps/glew/include)
	include_directories(${CMAKE_SOURCE_DIR}/ext-deps/devil-1.7.8/include)
endif (WIN32)

set (SPINTRO_SOURCES
	main.cpp

	planet.cpp
	star.cpp
	sp_character.cpp
	sp_player.cpp

	sp_camera.cpp
	sp_game.cpp
	sp_renderer.cpp
	sp_systems.cpp
	sp_window.cpp
)
add_executable (sp ${SPINTRO_SOURCES})
target_link_libraries (sp tinker tengine)
