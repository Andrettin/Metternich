cmake_minimum_required(VERSION 3.18.0)

project(metternich VERSION 1.0.0)

set(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo" CACHE STRING "" FORCE)

include_directories(
	src
)

set(map_SRCS
	src/map/province.cpp
)
source_group(map FILES ${map_SRCS})

set(metternich_SRCS
	${map_SRCS}
)

set(map_HDRS
	src/map/province.h
)
source_group(map FILES ${map_HDRS})

set(metternich_HDRS
	${map_HDRS}
	src/pch.h
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

find_package(Boost 1.69.0 REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
add_definitions(-DBOOST_DATE_TIME_NO_LIB)

#Qt5 modules
#different modules have different licenses, make sure all modules used here are compatible with the GPL 2.0
set(CMAKE_AUTOMOC ON)
find_package(Qt5 5.14 COMPONENTS Core REQUIRED) #licensed under the GPL 2.0 (as well as the LGPL 3.0)
find_package(Qt5 5.14 COMPONENTS Gui REQUIRED) #licensed under the GPL 2.0 (as well as the LGPL 3.0)
find_package(Qt5 5.14 COMPONENTS Widgets REQUIRED) #licensed under the GPL 2.0 (as well as the LGPL 3.0)
find_package(Qt5 5.14 COMPONENTS Multimedia REQUIRED) #licensed under the GPL 2.0 (as well as the LGPL 3.0)
find_package(Qt5 5.14 COMPONENTS Location REQUIRED) #licensed under the GPL 2.0 (as well as the LGPL 3.0)
find_package(Qt5 5.14 COMPONENTS Qml REQUIRED) #licensed under the GPL 2.0 (as well as the LGPL 3.0)
find_package(Qt5 5.14 COMPONENTS Quick REQUIRED) #licensed under the GPL 2.0 (as well as the LGPL 3.0)

set(QT_LIBRARIES
	Qt5::Core
	Qt5::Gui
	Qt5::Widgets
	Qt5::Multimedia
	Qt5::Location
	Qt5::Qml
	Qt5::Quick
)

set(QT_LIBRARIES ${QT_LIBRARIES} Qt5::LocationPrivate)

#find_package(SDL2 REQUIRED)
#find_package(SDL2_mixer REQUIRED)

#include_directories(
#	${SDL2_INCLUDE_DIR}
#	${SDL2_MIXER_INCLUDE_DIR}
#)

set(BINARY_NAME "metternich" CACHE PATH "Sets the name of the binary.")

set(metternich_LIBS
	${SDL2_LIBRARY}
	${SDL2_MIXER_LIBRARY}
	${QT_LIBRARIES}
)

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

#target_precompile_headers(metternich PRIVATE src/pch.h)

set_target_properties(metternich PROPERTIES UNITY_BUILD_MODE GROUP)

set_source_files_properties(${map_SRCS} PROPERTIES UNITY_GROUP "map")

set_target_properties(metternich_test PROPERTIES UNITY_BUILD_MODE GROUP)
set_source_files_properties(${game_test_SRCS} PROPERTIES UNITY_GROUP "game_test")

target_link_libraries(metternich ${metternich_LIBS})

target_link_libraries(metternich ${metternich_LIBS})

#target_precompile_headers(metternich_main REUSE_FROM metternich)
#target_precompile_headers(metternich_test REUSE_FROM metternich)

set_target_properties(metternich_main PROPERTIES OUTPUT_NAME ${BINARY_NAME})

target_link_libraries(metternich_main LINK_PUBLIC metternich)
target_link_libraries(metternich_test LINK_PUBLIC metternich)
