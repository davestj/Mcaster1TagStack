cmake_minimum_required(VERSION 3.21)
project(Mcaster1TagStackd VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(yaml-cpp REQUIRED)

# Placeholder target — Phase 2 will fill in src/
add_executable(tagstackd)
target_sources(tagstackd PRIVATE src/main.cpp)
target_link_libraries(tagstackd PRIVATE CURL::libcurl OpenSSL::SSL OpenSSL::Crypto yaml-cpp)
target_include_directories(tagstackd PRIVATE include)

install(TARGETS tagstackd RUNTIME DESTINATION bin)
