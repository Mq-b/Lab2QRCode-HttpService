cmake_minimum_required(VERSION 3.28)
project(Lab2QRCode-HttpService)

set(CMAKE_CXX_STANDARD 20)

find_package(libhv REQUIRED)
find_package(ZLIB REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(${PROJECT_NAME}
        ${SOURCES}
        main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} PRIVATE hv_static)
target_link_libraries(${PROJECT_NAME} PRIVATE ZLIB::ZLIB)
target_compile_definitions(${PROJECT_NAME} PRIVATE ZLIB_CONST)

if(MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_options(/EHsc /utf-8 /bigobj)
endif()

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
endif()
