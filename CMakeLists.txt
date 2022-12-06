cmake_minimum_required(VERSION 3.18.0)

project(metternich VERSION 1.0.0)

set(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo" CACHE STRING "" FORCE)

add_subdirectory(archimedes)

include_directories(
	archimedes/src
	src
	src/third_party
	src/third_party/maskedmousearea
	src/third_party/xbrz
)

set(country_SRCS
	src/country/consulate.cpp
	src/country/country.cpp
	src/country/country_container.cpp
	src/country/country_game_data.cpp
	src/country/country_history.cpp
	src/country/country_palette.cpp
	src/country/country_type.cpp
	src/country/cultural_group.cpp
	src/country/cultural_group_rank.cpp
	src/country/culture.cpp
	src/country/culture_base.cpp
	src/country/culture_container.cpp
	src/country/diplomacy_state.cpp
	src/country/landed_title.cpp
	src/country/landed_title_tier.cpp
)
source_group(country FILES ${country_SRCS})
set_source_files_properties(${country_SRCS} PROPERTIES UNITY_GROUP "country")

set(database_SRCS
	src/database/defines.cpp
	src/database/preferences.cpp
)
source_group(database FILES ${database_SRCS})
set_source_files_properties(${database_SRCS} PROPERTIES UNITY_GROUP "database")

set(economy_SRCS
	src/economy/commodity.cpp
	src/economy/commodity_container.cpp
	src/economy/employment_type.cpp
	src/economy/food_type.cpp
	src/economy/resource.cpp
	src/economy/resource_container.cpp
)
source_group(economy FILES ${economy_SRCS})
set_source_files_properties(${economy_SRCS} PROPERTIES UNITY_GROUP "economy")

set(game_SRCS
	src/game/game.cpp
	src/game/scenario.cpp
)
source_group(game FILES ${game_SRCS})
set_source_files_properties(${game_SRCS} PROPERTIES UNITY_GROUP "game")

set(infrastructure_SRCS
	src/infrastructure/building_class.cpp
	src/infrastructure/building_class_container.cpp
	src/infrastructure/building_slot.cpp
	src/infrastructure/building_slot_type_container.cpp
	src/infrastructure/building_type.cpp
	src/infrastructure/building_type_container.cpp
	src/infrastructure/improvement.cpp
)
source_group(infrastructure FILES ${infrastructure_SRCS})
set_source_files_properties(${infrastructure_SRCS} PROPERTIES UNITY_GROUP "infrastructure")

set(map_SRCS
	src/map/diplomatic_map_image_provider.cpp
	src/map/map.cpp
	src/map/map_generator.cpp
	src/map/map_grid_model.cpp
	src/map/map_template.cpp
	src/map/province.cpp
	src/map/province_container.cpp
	src/map/province_game_data.cpp
	src/map/province_history.cpp
	src/map/region.cpp
	src/map/region_history.cpp
	src/map/site.cpp
	src/map/site_container.cpp
	src/map/site_game_data.cpp
	src/map/site_history.cpp
	src/map/site_type.cpp
	src/map/terrain_adjacency.cpp
	src/map/terrain_feature.cpp
	src/map/terrain_geodata_map.cpp
	src/map/terrain_type.cpp
	src/map/terrain_type_container.cpp
	src/map/tile.cpp
	src/map/tile_image_provider.cpp
	src/map/world.cpp
)
source_group(map FILES ${map_SRCS})
set_source_files_properties(${map_SRCS} PROPERTIES UNITY_GROUP "map")

set(population_SRCS
	src/population/phenotype.cpp
	src/population/phenotype_container.cpp
	src/population/population_class.cpp
	src/population/population_class_container.cpp
	src/population/population_group_map.cpp
	src/population/population_type.cpp
	src/population/population_type_container.cpp
	src/population/population_unit.cpp
)
source_group(population FILES ${population_SRCS})
set_source_files_properties(${population_SRCS} PROPERTIES UNITY_GROUP "population")

set(technology_SRCS
	src/technology/technology.cpp
	src/technology/technology_container.cpp
)
source_group(technology FILES ${technology_SRCS})
set_source_files_properties(${technology_SRCS} PROPERTIES UNITY_GROUP "technology")

set(time_SRCS
	src/time/era.cpp
)
source_group(time FILES ${time_SRCS})
set_source_files_properties(${time_SRCS} PROPERTIES UNITY_GROUP "time")

set(ui_SRCS
	src/ui/icon.cpp
	src/ui/icon_container.cpp
	src/ui/icon_image_provider.cpp
	src/ui/interface_image_provider.cpp
)
source_group(ui FILES ${ui_SRCS})
set_source_files_properties(${ui_SRCS} PROPERTIES UNITY_GROUP "ui")

set(unit_SRCS
	src/unit/civilian_unit.cpp
	src/unit/civilian_unit_class.cpp
	src/unit/civilian_unit_class_container.cpp
	src/unit/civilian_unit_type.cpp
	src/unit/historical_civilian_unit.cpp
	src/unit/military_unit_class.cpp
	src/unit/military_unit_domain.cpp
)
source_group(unit FILES ${unit_SRCS})
set_source_files_properties(${unit_SRCS} PROPERTIES UNITY_GROUP "unit")

set(maskedmousearea_SRCS
	src/third_party/maskedmousearea/maskedmousearea.cpp
)
source_group(maskedmousearea FILES ${maskedmousearea_SRCS})
set_source_files_properties(${maskedmousearea_SRCS} PROPERTIES UNITY_GROUP "maskedmousearea")

set(xbrz_SRCS
	src/third_party/xbrz/xbrz.cpp
)
source_group(xbrz FILES ${xbrz_SRCS})
set_source_files_properties(${xbrz_SRCS} PROPERTIES UNITY_GROUP "xbrz")

set(metternich_SRCS
	${country_SRCS}
	${database_SRCS}
	${economy_SRCS}
	${game_SRCS}
	${infrastructure_SRCS}
	${map_SRCS}
	${population_SRCS}
	${technology_SRCS}
	${time_SRCS}
	${ui_SRCS}
	${unit_SRCS}
	${maskedmousearea_SRCS}
	${xbrz_SRCS}
	src/engine_interface.cpp
)

set(country_HDRS
	src/country/consulate.h
	src/country/country.h
	src/country/country_container.h
	src/country/country_game_data.h
	src/country/country_history.h
	src/country/country_palette.h
	src/country/country_type.h
	src/country/cultural_group.h
	src/country/cultural_group_rank.h
	src/country/culture.h
	src/country/culture_base.h
	src/country/culture_container.h
	src/country/diplomacy_state.h
	src/country/landed_title.h
	src/country/landed_title_tier.h
)
source_group(country FILES ${country_HDRS})

set(database_HDRS
	src/database/defines.h
	src/database/preferences.h
)
source_group(database FILES ${database_HDRS})

set(economy_HDRS
	src/economy/commodity.h
	src/economy/commodity_container.h
	src/economy/employment_type.h
	src/economy/food_type.h
	src/economy/resource.h
	src/economy/resource_container.h
)
source_group(economy FILES ${economy_HDRS})

set(game_HDRS
	src/game/game.h
	src/game/scenario.h
)
source_group(game FILES ${game_HDRS})

set(infrastructure_HDRS
	src/infrastructure/building_class.h
	src/infrastructure/building_class_container.h
	src/infrastructure/building_slot.h
	src/infrastructure/building_slot_type.h
	src/infrastructure/building_slot_type_container.h
	src/infrastructure/building_type.h
	src/infrastructure/building_type_container.h
	src/infrastructure/improvement.h
)
source_group(infrastructure FILES ${infrastructure_HDRS})

set(map_HDRS
	src/map/diplomatic_map_image_provider.h
	src/map/map.h
	src/map/map_generator.h
	src/map/map_grid_model.h
	src/map/map_template.h
	src/map/province.h
	src/map/province_container.h
	src/map/province_game_data.h
	src/map/province_history.h
	src/map/province_map_mode.h
	src/map/region.h
	src/map/region_history.h
	src/map/site.h
	src/map/site_container.h
	src/map/site_game_data.h
	src/map/site_history.h
	src/map/site_type.h
	src/map/terrain_adjacency.h
	src/map/terrain_adjacency_type.h
	src/map/terrain_feature.h
	src/map/terrain_geodata_map.h
	src/map/terrain_type.h
	src/map/terrain_type_container.h
	src/map/tile.h
	src/map/tile_image_provider.h
	src/map/world.h
)
source_group(map FILES ${map_HDRS})

set(population_HDRS
	src/population/phenotype.h
	src/population/phenotype_container.h
	src/population/population_class.h
	src/population/population_class_container.h
	src/population/population_group_map.h
	src/population/population_type.h
	src/population/population_type_container.h
	src/population/population_unit.h
)
source_group(population FILES ${population_HDRS})

set(technology_HDRS
	src/technology/technology.h
	src/technology/technology_container.h
)
source_group(technology FILES ${technology_HDRS})

set(time_HDRS
	src/time/era.h
)
source_group(time FILES ${time_HDRS})

set(ui_HDRS
	src/ui/icon.h
	src/ui/icon_container.h
	src/ui/icon_image_provider.h
	src/ui/interface_image_provider.h
)
source_group(ui FILES ${ui_HDRS})

set(unit_HDRS
	src/unit/civilian_unit.h
	src/unit/civilian_unit_class.h
	src/unit/civilian_unit_class_container.h
	src/unit/civilian_unit_type.h
	src/unit/historical_civilian_unit.h
	src/unit/historical_civilian_unit_history.h
	src/unit/military_unit_class.h
	src/unit/military_unit_domain.h
)
source_group(unit FILES ${unit_HDRS})

set(maskedmousearea_HDRS
	src/third_party/maskedmousearea/maskedmousearea.h
)
source_group(maskedmousearea FILES ${maskedmousearea_HDRS})

set(xbrz_HDRS
	src/third_party/xbrz/xbrz.h
	src/third_party/xbrz/xbrz_config.h
	src/third_party/xbrz/xbrz_tools.h
)
source_group(xbrz FILES ${xbrz_HDRS})

set(metternich_HDRS
	${country_HDRS}
	${database_HDRS}
	${economy_HDRS}
	${game_HDRS}
	${infrastructure_HDRS}
	${map_HDRS}
	${population_HDRS}
	${technology_HDRS}
	${time_HDRS}
	${ui_HDRS}
	${unit_HDRS}
	${maskedmousearea_HDRS}
	${xbrz_HDRS}
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
find_package(Qt5 5.14 COMPONENTS Charts REQUIRED) #licensed under the GPL 3.0

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

set(CMAKE_UNITY_BUILD ON)

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

target_link_libraries(metternich_main PUBLIC metternich Qt5::Charts)
target_link_libraries(metternich_test PUBLIC metternich)
