cmake_minimum_required(VERSION 3.16)
project(ColorSelector LANGUAGES CXX)

# -----------------------------------------------------------------------------
# Header-only library
# -----------------------------------------------------------------------------
add_library(ColorSelector INTERFACE)

target_include_directories(ColorSelector INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(ColorSelector INTERFACE cxx_std_17)

# -----------------------------------------------------------------------------
# Demo (optional, only built when this is the top-level project)
# -----------------------------------------------------------------------------
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    find_package(Qt6 6.0 REQUIRED COMPONENTS Core Gui Widgets)

    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)

    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)

    # Pull in the library sources so AUTOMOC can process them
    file(GLOB COLOR_SELECT_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/color_select/*.cpp
    )
    file(GLOB COLOR_SELECT_HEADERS CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/color_select/*.h
    )

    set(DEMO_SOURCES
        cosel_demo_widget.cpp
        main.cpp
        unit_test.cpp
    )

    set(DEMO_HEADERS
        cosel_demo_widget.h
        unit_test.h
    )

    qt_add_executable(ColorSelectorDemo
        ${DEMO_SOURCES}
        ${DEMO_HEADERS}
        ${COLOR_SELECT_SOURCES}
        ${COLOR_SELECT_HEADERS}
    )

    target_link_libraries(ColorSelectorDemo PRIVATE
        ColorSelector
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
    )

    target_compile_definitions(ColorSelectorDemo PRIVATE
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    )

    # Warnings (demo only)
    if(MSVC)
        target_compile_options(ColorSelectorDemo PRIVATE /W4 /wd4100 /wd4458)
    else()
        target_compile_options(ColorSelectorDemo PRIVATE
            -Wall -Wextra -Wno-reorder -Wno-unused-function
        )
    endif()

    # Optimizations (demo only)
    if(MSVC)
        target_compile_options(ColorSelectorDemo PRIVATE
            $<$<CONFIG:Debug>:/Od /Zi>
            $<$<CONFIG:Release>:/O2>
        )
    else()
        target_compile_options(ColorSelectorDemo PRIVATE
            $<$<CONFIG:Debug>:-O0>
            $<$<CONFIG:Release>:-O3>
        )
    endif()
endif()

# -----------------------------------------------------------------------------
# Install rules
# -----------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS ColorSelector
    EXPORT ColorSelectorTargets
)

install(DIRECTORY color_select/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/color_select
    FILES_MATCHING PATTERN "*.h"
)

if(TARGET ColorSelectorDemo)
    install(TARGETS ColorSelectorDemo
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()