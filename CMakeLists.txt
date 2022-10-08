cmake_minimum_required(VERSION 3.18.0)

project(metternich VERSION 1.0.0)

set(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo" CACHE STRING "" FORCE)

add_subdirectory(archimedes)

include_directories(
	archimedes/src
	src
)

set(country_SRCS
	src/country/country.cpp
	src/country/country_type.cpp
)
source_group(country FILES ${country_SRCS})
set_source_files_properties(${country_SRCS} PROPERTIES UNITY_GROUP "country")

set(database_SRCS
	src/database/defines.cpp
)
source_group(database FILES ${database_SRCS})
set_source_files_properties(${database_SRCS} PROPERTIES UNITY_GROUP "database")

set(game_SRCS
	src/game/game.cpp
)
source_group(game FILES ${game_SRCS})
set_source_files_properties(${game_SRCS} PROPERTIES UNITY_GROUP "game")

set(map_SRCS
	src/map/diplomatic_map_image_provider.cpp
	src/map/map.cpp
	src/map/map_grid_model.cpp
	src/map/map_template.cpp
	src/map/province.cpp
	src/map/province_container.cpp
	src/map/scenario.cpp
	src/map/site.cpp
	src/map/site_container.cpp
	src/map/site_type.cpp
	src/map/terrain_feature.cpp
	src/map/terrain_geodata_map.cpp
	src/map/terrain_type.cpp
	src/map/tile.cpp
	src/map/tile_image_provider.cpp
	src/map/world.cpp
)
source_group(map FILES ${map_SRCS})
set_source_files_properties(${map_SRCS} PROPERTIES UNITY_GROUP "map")

set(metternich_SRCS
	${country_SRCS}
	${database_SRCS}
	${game_SRCS}
	${map_SRCS}
	src/engine_interface.cpp
)

set(country_HDRS
	src/country/country.h
	src/country/country_type.h
)
source_group(country FILES ${country_HDRS})

set(database_HDRS
	src/database/defines.h
)
source_group(database FILES ${database_HDRS})

set(game_HDRS
	src/game/game.h
)
source_group(game FILES ${game_HDRS})

set(map_HDRS
	src/map/diplomatic_map_image_provider.h
	src/map/map.h
	src/map/map_grid_model.h
	src/map/map_template.h
	src/map/province.h
	src/map/province_container.h
	src/map/province_game_data.h
	src/map/province_history.h
	src/map/scenario.h
	src/map/site.h
	src/map/site_container.h
	src/map/site_type.h
	src/map/terrain_feature.h
	src/map/terrain_geodata_map.h
	src/map/terrain_type.h
	src/map/tile.h
	src/map/tile_image_provider.h
	src/map/world.h
)
source_group(map FILES ${map_HDRS})

set(metternich_HDRS
	${country_HDRS}
	${database_HDRS}
	${game_HDRS}
	${map_HDRS}
	src/engine_interface.h
	src/metternich.h
)

set(metternich_main_SRCS
	src/main.cpp
)

set(game_test_SRCS
#	test/game/game_test.cpp
)
source_group(game FILES ${game_test_SRCS})

set(metternich_test_SRCS
	${game_test_SRCS}
	test/main.cpp
)

option(WITH_GEOJSON "Compile with support for generating map data from GeoJSON files" OFF)

find_package(Boost 1.69.0 REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
add_definitions(-DBOOST_DATE_TIME_NO_LIB)

#Qt5 modules
#different modules have different licenses, make sure all modules used here are compatible with the MIT license
set(CMAKE_AUTOMOC ON)
find_package(Qt5 5.14 COMPONENTS Core REQUIRED) #licensed under the GPL 2.0, as well as the LGPL 3.0
find_package(Qt5 5.14 COMPONENTS Gui REQUIRED) #licensed under the GPL 2.0, as well as the LGPL 3.0
find_package(Qt5 5.14 COMPONENTS Widgets REQUIRED) #licensed under the GPL 2.0, as well as the LGPL 3.0
find_package(Qt5 5.14 COMPONENTS Multimedia REQUIRED) #licensed under the GPL 2.0, as well as the LGPL 3.0
find_package(Qt5 5.14 COMPONENTS Location REQUIRED) #licensed under the GPL 2.0, as well as the LGPL 3.0
find_package(Qt5 5.14 COMPONENTS Qml REQUIRED) #licensed under the GPL 2.0, as well as the LGPL 3.0
find_package(Qt5 5.14 COMPONENTS Quick REQUIRED) #licensed under the GPL 2.0, as well as the LGPL 3.0

if(WITH_GEOJSON)
	add_definitions(-DUSE_GEOJSON)
endif()

if(MSVC)
	add_definitions(/FI"metternich.h")
else()
	#GCC/Clang
	add_definitions(-include metternich.h)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
	add_compile_options(/W4 /w44800 /wd4244 /wd4458)
else()
	add_compile_options(-Wall -Wno-unknown-pragmas)
endif()

add_library(metternich STATIC ${metternich_SRCS} ${metternich_HDRS})
add_executable(metternich_main WIN32 ${metternich_main_SRCS})

add_executable(metternich_test ${metternich_test_SRCS})
add_test(metternich_test metternich_test)
enable_testing()

target_precompile_headers(metternich REUSE_FROM archimedes)

set_target_properties(metternich PROPERTIES UNITY_BUILD_MODE GROUP)

set_target_properties(metternich_test PROPERTIES UNITY_BUILD_MODE GROUP)
set_source_files_properties(${game_test_SRCS} PROPERTIES UNITY_GROUP "game_test")

target_link_libraries(metternich PUBLIC archimedes)

target_precompile_headers(metternich_main REUSE_FROM archimedes)
target_precompile_headers(metternich_test REUSE_FROM archimedes)

set_target_properties(metternich_main PROPERTIES OUTPUT_NAME "metternich")

target_link_libraries(metternich_main PUBLIC metternich)
target_link_libraries(metternich_test PUBLIC metternich)
