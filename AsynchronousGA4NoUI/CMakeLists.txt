cmake_minimum_required(VERSION 3.5)

# set some output folders
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/$<CONFIG>/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/$<CONFIG>/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/$<CONFIG>/lib)

project(AsynchronousGA4NoUI VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


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

