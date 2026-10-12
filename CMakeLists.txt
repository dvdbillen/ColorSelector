cmake_minimum_required(VERSION 3.16)
project(ColorSelectorDemo LANGUAGES CXX)

# Require Qt 6
find_package(Qt6 6.0 REQUIRED COMPONENTS Core Gui Widgets)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt automoc/uic/rcc (equivalent to what qmake does)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Optional modern shortcut (Qt 6.3+)
# qt_standard_project_setup()

# Header-only / library sources in color_select/
file(GLOB COLOR_SELECT_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/color_select/*.cpp
)
file(GLOB COLOR_SELECT_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/color_select/*.h
)

# Demo sources
set(DEMO_SOURCES
    cosel_demo_widget.cpp
    main.cpp
    unit_test.cpp
)

set(DEMO_HEADERS
    cosel_demo_widget.h
    unit_test.h
)

# Main executable
qt_add_executable(ColorSelectorDemo
    ${DEMO_SOURCES}
    ${DEMO_HEADERS}
    ${COLOR_SELECT_SOURCES}
    ${COLOR_SELECT_HEADERS}
)

target_link_libraries(ColorSelectorDemo PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Defines from the .pro
target_compile_definitions(ColorSelectorDemo PRIVATE
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
)

# Warning flags
target_compile_options(ColorSelectorDemo PRIVATE
    -Wno-reorder
    -Wno-unused-function
    -Wfatal-errors
)

# Optimization flags (cleaned up a bit from the mixed release block)
# Debug
target_compile_options(ColorSelectorDemo PRIVATE
    $<$<CONFIG:Debug>:-O0>
)

# Release
target_compile_options(ColorSelectorDemo PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Release>:-march=native>
    $<$<CONFIG:Release>:-mtune=native>
    $<$<CONFIG:Release>:-flto>
    $<$<CONFIG:Release>:-funroll-loops>
    $<$<CONFIG:Release>:-ftree-vectorize>
    $<$<CONFIG:Release>:-fno-math-errno>
    $<$<CONFIG:Release>:-funsafe-math-optimizations>
    $<$<CONFIG:Release>:-fno-signed-zeros>
)

target_link_options(ColorSelectorDemo PRIVATE
    $<$<CONFIG:Release>:-flto>
)

# Optional install rules (simplified from the .pro)
include(GNUInstallDirs)
install(TARGETS ColorSelectorDemo
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)