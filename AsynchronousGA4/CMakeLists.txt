cmake_minimum_required(VERSION 3.5)

project(AsynchronousGA4 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
    BasicXMLSyntaxHighlighter.cpp
    GAParametersDialog.cpp
    MainWindow.cpp
    TextEditDialog.cpp
    Utilities.cpp
    main.cpp

    BasicXMLSyntaxHighlighter.h
    GAParametersDialog.h
    MainWindow.h
    TextEditDialog.h
    Utilities.h

    GAParametersDialog.ui
    MainWindow.ui
    TextEditDialog.ui

    Resources.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(AsynchronousGA4
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET AsynchronousGA4 APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(AsynchronousGA4 SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(AsynchronousGA4
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(AsynchronousGA4 PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

find_package(Qt6 REQUIRED COMPONENTS Xml)
target_link_libraries(AsynchronousGA4 PRIVATE Qt6::Xml)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.AsynchronousGA4)
endif()
set_target_properties(AsynchronousGA4 PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS AsynchronousGA4
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

target_include_directories(AsynchronousGA4 PRIVATE
    ../pystring
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(AsynchronousGA4)
endif()
