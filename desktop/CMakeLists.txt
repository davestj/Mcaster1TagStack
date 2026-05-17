cmake_minimum_required(VERSION 3.21)
project(Mcaster1TagStackDesktop VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Qt6 — prefer Homebrew on macOS; system-installed elsewhere
find_package(Qt6 6.7 COMPONENTS Core Widgets Network REQUIRED)

# TagLib for audio metadata reading
find_package(TagLib QUIET)

# Placeholder target — Phase 3 will populate src/
add_executable(tagstack-desktop)
target_sources(tagstack-desktop PRIVATE src/main.cpp)
target_link_libraries(tagstack-desktop PRIVATE Qt6::Core Qt6::Widgets Qt6::Network)

if(APPLE)
    set_target_properties(tagstack-desktop PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER com.mcaster1.tagstack
        MACOSX_BUNDLE_BUNDLE_NAME "Mcaster1 TagStack"
    )
endif()
