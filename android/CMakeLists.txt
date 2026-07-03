cmake_minimum_required(VERSION 3.22)
project(Obentou C)

if(NOT ANDROID)
    message(FATAL_ERROR "This CMakeLists.txt is for the Android build only; use the Makefile for Windows/Linux/macOS/Emscripten.")
endif()

set(OBENTOU_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

include(FetchContent)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG 655c816
)

FetchContent_Declare(
    SDL3_ttf
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
    GIT_TAG release-3.2.2
)

FetchContent_MakeAvailable(SDL3 SDL3_ttf)

# SRC := $(shell find src ext -name '*.c')
file(GLOB_RECURSE SRC CONFIGURE_DEPENDS
    ${OBENTOU_DIR}/src/*.c
    ${OBENTOU_DIR}/ext/*.c
)

# converted_assets/%.h: assets/%  ->  xxd -i $< > $@
file(GLOB_RECURSE RAW_ASSETS CONFIGURE_DEPENDS LIST_DIRECTORIES false
    ${OBENTOU_DIR}/assets/*
)
set(GENERATED_HEADERS "")
foreach(asset ${RAW_ASSETS})
    file(RELATIVE_PATH asset_rel "${OBENTOU_DIR}/assets" "${asset}")
    set(header "${OBENTOU_DIR}/converted_assets/${asset_rel}.h")
    get_filename_component(header_dir "${header}" DIRECTORY)
    
    file(MAKE_DIRECTORY "${header_dir}")
    
    add_custom_command(
        OUTPUT "${header}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${header_dir}"
        # Let xxd write to the output file natively! No "sh -c" or ">" required.
        COMMAND xxd -i "assets/${asset_rel}" "${header}"
        DEPENDS "${asset}"
        WORKING_DIRECTORY "${OBENTOU_DIR}"
    )
    list(APPEND GENERATED_HEADERS "${header}")
endforeach()
add_custom_target(convert_assets DEPENDS ${GENERATED_HEADERS})

# Android needs a SHARED lib literally named "main" (SDLActivity loads libmain.so)
add_library(main SHARED ${SRC})
add_dependencies(main convert_assets)

# CFLAGS_COMMON := -Iconverted_assets -Iinclude -Iext/include -O3
target_include_directories(main PRIVATE
    ${OBENTOU_DIR}/converted_assets
    ${OBENTOU_DIR}/include
    ${OBENTOU_DIR}/ext/include
)
target_compile_options(main PRIVATE -O3)

target_link_libraries(main PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf android log)