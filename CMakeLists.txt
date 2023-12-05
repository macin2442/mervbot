cmake_minimum_required(VERSION 3.16)

project(MERVBot)

find_package(Boost COMPONENTS program_options REQUIRED)
if(NOT Boost_FOUND)
	message(error ": Boost library not found.")
endif()
if (UNIX)
	add_compile_definitions(__stdcall=)
endif()
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS})
link_libraries(${Boost_LIBRARIES})

add_executable(MERVBot
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

include_directories(${PROJECT_SOURCE_DIR}/src)
