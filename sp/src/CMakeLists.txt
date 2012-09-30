include_directories (
	${PROJECT_SOURCE_DIR}/../ext-deps/EASTL/include
	${CMAKE_SOURCE_DIR}/
	${CMAKE_SOURCE_DIR}/common
	${CMAKE_SOURCE_DIR}/common/math
	${CMAKE_SOURCE_DIR}/tinker
	${CMAKE_SOURCE_DIR}/tengine
	${CMAKE_SOURCE_DIR}/sp/src/
)

if (WIN32)
	include_directories(${CMAKE_SOURCE_DIR}/../ext-deps/gl3w/include)
	include_directories(${CMAKE_SOURCE_DIR}/../ext-deps/pthreads-w32-2-8-0-release)
endif (WIN32)

set (CI_PLANET_SOURCES
	planet/planet.cpp
	planet/planet_terrain.cpp
	planet/terrain_lumps.cpp
	planet/terrain_chunks.cpp
)

set (CI_ENTITY_SOURCES
	entities/star.cpp
	entities/sp_character.cpp
	entities/sp_playercharacter.cpp
	entities/sp_player.cpp
	entities/sp_camera.cpp
	entities/sp_game.cpp
	entities/sp_entity.cpp
)

set (CI_BOT_SOURCES
	entities/bots/bot.cpp
	entities/bots/helper.cpp
	entities/bots/worker.cpp
)

set (CI_STRUCTURE_SOURCES
	entities/structures/structure.cpp
	entities/structures/spire.cpp
)

set (CI_UI_SOURCES
	ui/hud.cpp
	ui/hudmenu.cpp
)

set (CI_SOURCES
	main.cpp

	sp_common.cpp
	sp_renderer.cpp
	sp_systems.cpp
	sp_window.cpp

	${CI_PLANET_SOURCES}
	${CI_ENTITY_SOURCES}
	${CI_STRUCTURE_SOURCES}
	${CI_BOT_SOURCES}
	${CI_UI_SOURCES}
)

source_group(Planet FILES ${CI_PLANET_SOURCES})
source_group(Entities FILES ${CI_ENTITY_SOURCES})
source_group(Entities\\Bots FILES ${CI_BOT_SOURCES})
source_group(Entities\\Structures FILES ${CI_STRUCTURE_SOURCES})
source_group(UI FILES ${CI_UI_SOURCES})

add_executable (infinite ${CI_SOURCES})
target_link_libraries (infinite tinker tengine)

if (WIN32)
	target_link_libraries(infinite debug ${CMAKE_SOURCE_DIR}/../ext-deps/pthreads-w32-2-8-0-release/Debug/pthread.lib)

	target_link_libraries(infinite optimized ${CMAKE_SOURCE_DIR}/../ext-deps/pthreads-w32-2-8-0-release/Release/pthread.lib)
endif (WIN32)
