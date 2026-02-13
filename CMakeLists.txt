cmake_minimum_required(VERSION 3.20)

project(charted
    VERSION      1.0.0
    LANGUAGES    CXX
    HOMEPAGE_URL "charted.ljyc.me"
)

option(CHARTED_BUILD_EXAMPLES "Build examples"              ON)
option(CHARTED_ENABLE_MODULES "Build C++20 module bindings" OFF)

add_library(charted INTERFACE)
target_compile_features(charted INTERFACE cxx_std_20)
target_include_directories(charted INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

set(CHARTED_SELECTED_TARGET charted)

if (CHARTED_ENABLE_MODULES)
    if (CMAKE_VERSION VERSION_LESS "3.28")
        message(WARNING "CHARTED_ENABLE_MODULES requires CMake >= 3.28. Module bindings target will be skipped.")
    else()
        add_library(charted_modules)
        target_compile_features(charted_modules PUBLIC cxx_std_20)
        target_include_directories(charted_modules PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        )
        target_link_libraries(charted_modules PUBLIC charted)
        target_sources(charted_modules PUBLIC
            FILE_SET "charted_modules"
            TYPE     CXX_MODULES
            FILES
                module/charted.cppm
                module/charted_json.cppm
        )

        set(CHARTED_SELECTED_TARGET charted_modules)
    endif()
endif()

add_library(charted::charted ALIAS ${CHARTED_SELECTED_TARGET})

if (CHARTED_BUILD_EXAMPLES)
    add_executable(charted_example examples/overview.cpp)
    target_link_libraries(charted_example PRIVATE charted::charted)

    if (CHARTED_ENABLE_MODULES AND NOT CMAKE_VERSION VERSION_LESS "3.28")
        add_executable(charted_module_example)
        target_compile_features(charted_module_example PRIVATE cxx_std_20)
        target_link_libraries(charted_module_example PRIVATE charted::charted)
        target_sources(charted_module_example
            PRIVATE
                examples/charted_module_main.cpp
            PUBLIC
                FILE_SET CXX_MODULES FILES
                    examples/charted_module.cppm
        )
    endif()
endif()
