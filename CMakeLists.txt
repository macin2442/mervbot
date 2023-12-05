cmake_minimum_required(VERSION 3.16)

project(MERVBot)

add_subdirectory(dep/zlib)

#find_package(Boost COMPONENTS program_options REQUIRED)
#if(NOT Boost_FOUND)
#	message(error ": Boost library not found.")
#endif()
#include_directories(${Boost_INCLUDE_DIR})
#link_directories(${Boost_LIBRARY_DIRS})
#link_libraries(${Boost_LIBRARIES})

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

if (UNIX)
	target_sources(MERVBot PRIVATE src/windows_compat.cpp src/winsock_compat.cpp)
	target_compile_definitions(MERVBot PRIVATE __stdcall=)
endif()

target_include_directories(MERVBot PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_include_directories(MERVBot SYSTEM PRIVATE dep/zlib $<TARGET_FILE_DIR:zlib>)
target_link_libraries(MERVBot zlib)

#find_package(ZLIB REQUIRED)
##link_directories(zlib)
#target_include_directories(MERVBot SYSTEM PUBLIC ${ZLIB_INCLUDE_DIRS})
#target_link_libraries(MERVBot ${ZLIB_LIBRARIES})
#target_link_libraries(MERVBot zlib)