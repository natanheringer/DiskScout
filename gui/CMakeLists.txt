cmake_minimum_required(VERSION 3.16)
project(DiskScoutGUI)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# Enable Qt MOC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Include directories
include_directories(../src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Source files
set(GUI_SOURCES
    main.cpp
    mainwindow.cpp
    models/filesystemmodel.cpp
    widgets/sunburstwidget.cpp
    widgets/treemapwidget.cpp
    scanner_wrapper.cpp
)

set(GUI_HEADERS
    mainwindow.h
    models/filesystemmodel.h
    widgets/sunburstwidget.h
    widgets/treemapwidget.h
    scanner_wrapper.h
)

# Create executable
add_executable(diskscout_gui ${GUI_SOURCES} ${GUI_HEADERS})

# Link Qt libraries
target_link_libraries(diskscout_gui 
    Qt6::Core 
    Qt6::Widgets
)

# Link with existing C backend
target_link_libraries(diskscout_gui 
    ../src/scanner.c
    ../src/cache.c
    ../src/main.c
    ../disk_assembler.o
)

# Compiler flags
target_compile_definitions(diskscout_gui PRIVATE QT_DEPRECATED_WARNINGS)
target_compile_options(diskscout_gui PRIVATE -O3 -Wall)

# Windows specific
if(WIN32)
    target_compile_definitions(diskscout_gui PRIVATE WIN32_LEAN_AND_MEAN)
    target_link_libraries(diskscout_gui pthread)
endif()
