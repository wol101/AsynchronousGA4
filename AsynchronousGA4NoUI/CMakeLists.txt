cmake_minimum_required(VERSION 3.5)

# set some output folders
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/$<CONFIG>/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/$<CONFIG>/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/$<CONFIG>/lib)

project(AsynchronousGA4NoUI VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
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
endif()

if (MSVC)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
add_compile_definitions(_CRT_SECURE_NO_WARNINGS _WINSOCK_DEPRECATED_NO_WARNINGS _WIN32_WINNT=_WIN32_WINNT_WIN7)
endif ()


# generates detailed compile_commands.json file
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(AsynchronousGA4NoUI
    ../AsynchronousGA4/MergeXML.cpp
    ../AsynchronousGA4/XMLContainer.cpp
    ../AsynchronousGA4/XMLContainerList.cpp
    ../AsynchronousGA4/XMLConverter.cpp
    ../src/ArgParse.cpp
    main.cpp
    Wrapper.cpp

    ../AsynchronousGA4/MergeUtil.h
    ../AsynchronousGA4/MergeXML.h
    ../AsynchronousGA4/XMLContainer.h
    ../AsynchronousGA4/XMLContainerList.h
    ../AsynchronousGA4/XMLConverter.h
    ../src/ArgParse.h
    Wrapper.h

    ../pocketpy/pocketpy.h
    ../pystring/pystring.cpp
    ../pystring/pystring.h
)


target_include_directories(AsynchronousGA4NoUI PRIVATE
    .
    ../AsynchronousGA4
    ../src
    ../pocketpy
    ../pystring
    ../rapidxml-1.13
)

# options for static build
set_target_properties(AsynchronousGA4NoUI PROPERTIES LINK_SEARCH_START_STATIC TRUE)
set_target_properties(AsynchronousGA4NoUI PROPERTIES LINK_SEARCH_END_STATIC TRUE)

