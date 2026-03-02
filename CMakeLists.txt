cmake_minimum_required(VERSION 3.20)
project(PlaneFight LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(raylib CONFIG REQUIRED)

add_executable(PlaneFight main.cpp)
target_link_libraries(PlaneFight PRIVATE raylib)

if(MSVC)
    target_compile_options(PlaneFight PRIVATE /utf-8)
endif()

set(PLANEFIGHT_ASSETS
    laserBlue.png
    laserRed.png
    playerShip_blue.png
    playerShip_red.png
    purple.png
)

foreach(asset IN LISTS PLANEFIGHT_ASSETS)
    add_custom_command(TARGET PlaneFight POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/${asset}"
            "$<TARGET_FILE_DIR:PlaneFight>/${asset}"
    )
endforeach()
