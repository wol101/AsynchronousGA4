cmake_minimum_required(VERSION 3.5)

# set some output folders
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/$<CONFIG>/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/$<CONFIG>/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/$<CONFIG>/lib)

project(AsynchronousGA4CL LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static builds (the default) link everything, including libc, into the
# executable so it can run unmodified across HPC nodes with mismatched
# shared library versions. Some deployments need the opposite: e.g. running
# under DMTCP, which injects its checkpoint runtime via LD_PRELOAD and
# therefore requires a real dynamic linker to be present, which a fully
# static executable does not have. Configure with -DSTATIC_BUILD=OFF to get
# a dynamically linked build for those cases; plain cmake keeps the current
# fully-static default.
option(STATIC_BUILD "Link the executable statically (including libc)" ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
# try to maximise portability
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -march=x86-64 -mtune=generic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=x86-64 -mtune=generic")
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)
add_link_options(-no-pie)
add_link_options(-static-libgcc -static-libstdc++)
if (STATIC_BUILD)
# options for static build
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(CMAKE_EXE_LINKER_FLAGS "-static")
set(CMAKE_EXE_LINK_DYNAMIC_C_FLAGS "")   # remove -rdynamic
set(CMAKE_EXE_LINK_DYNAMIC_CXX_FLAGS "")
endif()
endif()

if (MSVC)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
add_compile_definitions(_CRT_SECURE_NO_WARNINGS _WINSOCK_DEPRECATED_NO_WARNINGS _WIN32_WINNT=_WIN32_WINNT_WIN7)
endif ()

# generates detailed compile_commands.json file
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(AsynchronousGA4CL
    ../src/ArgParse.cpp
    ../src/DataFile.cpp
    ../src/GAASIO.cpp
    ../src/Genome.cpp
    ../src/MD5.cpp
    ../src/Mating.cpp
    ../src/Population.cpp
    ../src/Preferences.cpp
    ../src/Random.cpp
    ../src/ServerASIO.cpp
    ../src/Statistics.cpp
    ../pystring/pystring.cpp
    ../src/ArgParse.h
    ../src/DataFile.h
    ../src/GAASIO.h
    ../src/Genome.h
    ../src/MD5.h
    ../src/Mating.h
    ../src/Population.h
    ../src/Preferences.h
    ../src/Random.h
    ../src/ServerASIO.h
    ../src/Statistics.h
    ../asio-1.18.2/include/asio.hpp
    ../pystring/pystring.h
)

add_executable(RandomTest
    ../src/Random.cpp
    ../src/Random.h
    ../tests/RandomTest.cpp
)

target_include_directories(AsynchronousGA4CL PRIVATE
    ../src
    ../asio-1.18.2/include
    ../pystring
)

# make the linker prefer/require static archives when STATIC_BUILD is on
# (see the STATIC_BUILD option near the top of this file)
if (STATIC_BUILD)
set_target_properties(AsynchronousGA4CL PROPERTIES LINK_SEARCH_START_STATIC TRUE)
set_target_properties(AsynchronousGA4CL PROPERTIES LINK_SEARCH_END_STATIC TRUE)
endif()
