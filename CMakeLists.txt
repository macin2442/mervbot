cmake_minimum_required(VERSION 3.16)

project(MERVBot)

# generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(dep/zlib)

configure_file(MERVBot.INI MERVBot.INI COPYONLY)
configure_file(cfg/Commands.txt Commands.txt COPYONLY)
configure_file(cfg/Operators.txt Operators.txt COPYONLY)
configure_file(cfg/Spawns.txt Spawns.txt COPYONLY)

add_executable(${PROJECT_NAME}
	src/algorithms.cpp
	src/botdb.cpp
	src/botdll.cpp
	src/botinfo.cpp
	src/checksum.cpp
	src/chunk.cpp
	src/client.cpp
	src/clientprot.cpp
	src/command.cpp
	src/commtypes.cpp
	src/datatypes.cpp
	src/dllcore.cpp
	src/encrypt.cpp
	src/hack.cpp
	src/host.cpp
	src/hostlist.cpp
	src/map.cpp
	src/player.cpp
	src/prng.cpp
	src/sockets.cpp
	src/specialprot.cpp
	src/system.cpp
)

if (UNIX)
	find_package(Boost COMPONENTS filesystem program_options iostreams REQUIRED)
	if(NOT Boost_FOUND)
		message(error ": Boost library not found.")
	endif()

	target_sources(${PROJECT_NAME} PRIVATE src/windows_compat.cpp src/winsock_compat.cpp)
	target_compile_definitions(${PROJECT_NAME} PRIVATE __stdcall=)
	target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

	target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIR})
	target_link_directories(${PROJECT_NAME} PRIVATE ${Boost_LIBRARY_DIRS})
	target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
	target_link_libraries(${PROJECT_NAME} dl)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/src)
#target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE dep/zlib $<TARGET_FILE_DIR:zlib> $<TARGET_FILE_DIR:zlib>/..)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE dep/zlib $<TARGET_PROPERTY:zlibstatic,INCLUDE_DIRECTORIES>)

#file(GENERATE OUTPUT zlib_paths.txt CONTENT $<PATH:REMOVE_FILENAME,$<TARGET_PROPERTY:zlib,SOURCES>>)
#file(GENERATE OUTPUT zlib_paths.txt CONTENT $<TARGET_PROPERTY:zlib,INCLUDE_DIRECTORIES>)

target_link_libraries(${PROJECT_NAME} zlibstatic)

if (UNIX)
	add_executable(test
		src/windows_compat.cpp
		src/test.cpp
		src/datatypes.cpp
		src/algorithms.cpp
	)

	target_compile_definitions(test PRIVATE __stdcall=)
	target_compile_features(test PRIVATE cxx_std_17)

	target_include_directories(test PRIVATE ${Boost_INCLUDE_DIR})
	target_link_directories(test PRIVATE ${Boost_LIBRARY_DIRS})
	target_link_libraries(test ${Boost_LIBRARIES})

	target_link_libraries(test dl)
endif()
