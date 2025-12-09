cmake_minimum_required(VERSION 3.15)
project(Lab2QRCode-HttpService)

set(CMAKE_CXX_STANDARD 20)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/bin)

find_package(libhv REQUIRED)
find_package(ZLIB REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(${PROJECT_NAME}
        ${SOURCES}
        main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} PRIVATE 
    hv
    ZLIB::ZLIB
)

target_compile_definitions(${PROJECT_NAME} PRIVATE ZLIB_CONST)

if(MSVC)
    add_compile_options(/EHsc /utf-8 /bigobj)
endif()

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
endif()
