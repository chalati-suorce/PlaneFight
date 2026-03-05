cmake_minimum_required(VERSION 3.20)
project(PlaneFight LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(DEFINED VCPKG_TARGET_TRIPLET AND VCPKG_TARGET_TRIPLET MATCHES "-static$")
    set(raylib_USE_STATIC_LIBS ON)
endif()

find_package(raylib CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

# vcpkg's raylib port ships a find-module style "raylib-config.cmake" which is not
# multi-config aware. With Visual Studio it may pick the Debug .lib even for Release.
# Force per-config locations when building with vcpkg toolchain variables available.
if(TARGET raylib AND MSVC AND CMAKE_CONFIGURATION_TYPES AND DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
    set(_RAYLIB_ROOT "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
    if(VCPKG_TARGET_TRIPLET MATCHES "-static$")
        set_target_properties(raylib PROPERTIES
            IMPORTED_LOCATION_DEBUG "${_RAYLIB_ROOT}/debug/lib/raylib.lib"
            IMPORTED_LOCATION_RELEASE "${_RAYLIB_ROOT}/lib/raylib.lib"
            IMPORTED_LOCATION_RELWITHDEBINFO "${_RAYLIB_ROOT}/lib/raylib.lib"
            IMPORTED_LOCATION_MINSIZEREL "${_RAYLIB_ROOT}/lib/raylib.lib"
            IMPORTED_IMPLIB_DEBUG "${_RAYLIB_ROOT}/debug/lib/raylib.lib"
            IMPORTED_IMPLIB_RELEASE "${_RAYLIB_ROOT}/lib/raylib.lib"
            IMPORTED_IMPLIB_RELWITHDEBINFO "${_RAYLIB_ROOT}/lib/raylib.lib"
            IMPORTED_IMPLIB_MINSIZEREL "${_RAYLIB_ROOT}/lib/raylib.lib"
        )
    else()
        set_target_properties(raylib PROPERTIES
            IMPORTED_IMPLIB_DEBUG "${_RAYLIB_ROOT}/debug/lib/raylib.lib"
            IMPORTED_LOCATION_DEBUG "${_RAYLIB_ROOT}/debug/bin/raylib.dll"
            IMPORTED_IMPLIB_RELEASE "${_RAYLIB_ROOT}/lib/raylib.lib"
            IMPORTED_LOCATION_RELEASE "${_RAYLIB_ROOT}/bin/raylib.dll"
            IMPORTED_IMPLIB_RELWITHDEBINFO "${_RAYLIB_ROOT}/lib/raylib.lib"
            IMPORTED_LOCATION_RELWITHDEBINFO "${_RAYLIB_ROOT}/bin/raylib.dll"
            IMPORTED_IMPLIB_MINSIZEREL "${_RAYLIB_ROOT}/lib/raylib.lib"
            IMPORTED_LOCATION_MINSIZEREL "${_RAYLIB_ROOT}/bin/raylib.dll"
        )
    endif()
    unset(_RAYLIB_ROOT)
endif()

add_executable(PlaneFight
    main.cpp
    embedded_assets.cpp
    resources.rc
)
target_link_libraries(PlaneFight PRIVATE
    raylib
    glfw
    winmm
    gdi32
    user32
    shell32
    opengl32
    ole32
)

if(MSVC)
    target_compile_options(PlaneFight PRIVATE /utf-8)
    set_property(TARGET PlaneFight PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
