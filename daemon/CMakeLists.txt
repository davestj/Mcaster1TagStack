cmake_minimum_required(VERSION 3.21)
project(Mcaster1TagStackd VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Position-independent code + warnings
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
add_compile_options(-Wall -Wextra -Wpedantic)

# ── Dependencies ───────────────────────────────────────────────────────────
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

# yaml-cpp — CMake support varies; try find_package first, fall back to pkg-config
find_package(yaml-cpp QUIET)
if(NOT yaml-cpp_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(YAMLCPP REQUIRED yaml-cpp)
endif()

# MariaDB / MySQL client — Debian ships libmariadb-dev with mariadb_config
find_package(PkgConfig REQUIRED)
pkg_check_modules(MARIADB libmariadb)
if(NOT MARIADB_FOUND)
    pkg_check_modules(MARIADB REQUIRED mariadb)
endif()

# ── Target ─────────────────────────────────────────────────────────────────
add_executable(tagstackd
    src/main.cpp
)

target_include_directories(tagstackd PRIVATE
    include
    ${YAMLCPP_INCLUDE_DIRS}
    ${MARIADB_INCLUDE_DIRS}
)

target_link_libraries(tagstackd PRIVATE
    Threads::Threads
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    ${MARIADB_LIBRARIES}
)

target_link_directories(tagstackd PRIVATE ${MARIADB_LIBRARY_DIRS})

if(yaml-cpp_FOUND)
    target_link_libraries(tagstackd PRIVATE yaml-cpp)
else()
    target_link_libraries(tagstackd PRIVATE ${YAMLCPP_LIBRARIES})
    target_compile_options(tagstackd PRIVATE ${YAMLCPP_CFLAGS_OTHER})
endif()

# cpp-httplib needs CPPHTTPLIB_OPENSSL_SUPPORT — main.cpp #defines it,
# but we also need OpenSSL include dirs and ZLIB optional support.
find_package(ZLIB QUIET)
if(ZLIB_FOUND)
    target_link_libraries(tagstackd PRIVATE ZLIB::ZLIB)
    target_compile_definitions(tagstackd PRIVATE CPPHTTPLIB_ZLIB_SUPPORT)
endif()

install(TARGETS tagstackd RUNTIME DESTINATION bin)
