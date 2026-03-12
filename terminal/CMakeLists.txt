cmake_minimum_required(VERSION 3.5)

project(AsynchronousGA4CL LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# try to maximise portability
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -march=x86-64 -mtune=generic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=x86-64 -mtune=generic")
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)
add_link_options(-no-pie)

# options for static build
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(CMAKE_EXE_LINKER_FLAGS "-static")
add_link_options(-static-libgcc -static-libstdc++)
set(CMAKE_EXE_LINK_DYNAMIC_C_FLAGS "")   # remove -rdynamic
set(CMAKE_EXE_LINK_DYNAMIC_CXX_FLAGS "")

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

if (MSVC)
    target_compile_options(AsynchronousGA4CL PRIVATE /bigobj /MT) # /MT allows multithreading
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS _WINSOCK_DEPRECATED_NO_WARNINGS _WIN32_WINNT=_WIN32_WINNT_WIN7)
endif ()

# options for static build
set_target_properties(AsynchronousGA4CL PROPERTIES LINK_SEARCH_START_STATIC TRUE)
set_target_properties(AsynchronousGA4CL PROPERTIES LINK_SEARCH_END_STATIC TRUE)
