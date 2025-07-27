cmake_minimum_required(VERSION 3.26)
project(TonicTroubleFix VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_library(${PROJECT_NAME}_module SHARED src/module/Main.cpp
        src/module/pch.h
        src/module/Init.cpp
        external/ModUtils/Patterns.cpp)

find_package(safetyhook CONFIG REQUIRED)

set_target_properties(${PROJECT_NAME}_module PROPERTIES OUTPUT_NAME "msacm32" CXX_STANDARD 23 CXX_EXTENSIONS OFF)
target_precompile_headers(${PROJECT_NAME}_module PRIVATE src/module/pch.h)
target_compile_features(${PROJECT_NAME}_module PRIVATE cxx_std_23)
target_include_directories(${PROJECT_NAME}_module PRIVATE "external/ModUtils")
target_compile_definitions(${PROJECT_NAME}_module PRIVATE "_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING")
target_link_libraries(${PROJECT_NAME}_module PRIVATE pathcch shlwapi safetyhook::safetyhook)

add_executable(${PROJECT_NAME}_config WIN32 src/config-cpp/Main.cpp
        src/config-cpp/DialogApp.cpp
        src/config-cpp/MyDialog.cpp
        src/config-cpp/StdAfx.cpp
        src/config-cpp/DirectX.cpp
        src/config-cpp/DirectX.h
        src/config-cpp/IniFile.cpp
        src/config-cpp/IniFile.h)
set_target_properties(${PROJECT_NAME}_config PROPERTIES OUTPUT_NAME "ttconfig" CXX_STANDARD 20 CXX_EXTENSIONS OFF)
target_precompile_headers(${PROJECT_NAME}_config PRIVATE src/config-cpp/pch.h)
target_sources(${PROJECT_NAME}_config PRIVATE src/config-cpp/Resource.rc src/config-cpp/PerMonitorV2.manifest)
target_compile_features(${PROJECT_NAME}_config PRIVATE cxx_std_20)
target_compile_definitions(${PROJECT_NAME}_config PRIVATE UNICODE _UNICODE)
target_include_directories(${PROJECT_NAME}_config PRIVATE "external/Win32xx/include")
target_link_libraries(${PROJECT_NAME}_config PRIVATE comctl32 pathcch shlwapi ddraw d3d9 dxguid)
