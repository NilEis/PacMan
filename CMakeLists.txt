cmake_minimum_required(VERSION 3.24) # Set the minimum required CMake version
project(pacman C)

function(add_header path precompile_header)
    get_filename_component(name "${path}" NAME)

    list(APPEND HEADERS "${head_path}")
    if (${precompile_header})
        list(APPEND PRECOMPILED_HEADERS "${head_path}")
        message(STATUS "Add header: ${name} - precompiled")
    else (${precompile_header})
        message(STATUS "Add header: ${name}")
    endif (${precompile_header})

    set(HEADERS
            ${HEADERS}
            PARENT_SCOPE)
    set(PRECOMPILED_HEADERS
            ${PRECOMPILED_HEADERS}
            PARENT_SCOPE)
endfunction(add_header)

function(add_source folder name precompile_header)
    cmake_path(SET src_path_start NORMALIZE "${folder}/src/")
    cmake_path(SET head_path_start NORMALIZE "${folder}/include/")

    cmake_path(APPEND src_path "${src_path_start}" "${name}.c")
    cmake_path(APPEND head_path "${head_path_start}" "${name}.h")

    list(APPEND SOURCES "${src_path}")

    add_header(${head_path} ${precompile_header})

    set(SOURCES
            ${SOURCES}
            PARENT_SCOPE)
    set(HEADERS
            ${HEADERS}
            PARENT_SCOPE)
    set(PRECOMPILED_HEADERS
            ${PRECOMPILED_HEADERS}
            PARENT_SCOPE)
endfunction(add_source)

option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)

include(FetchContent)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/options.cmake)

create_resources(
        ${CMAKE_CURRENT_SOURCE_DIR}/src/assets "asset_"
        ${CMAKE_CURRENT_BINARY_DIR}/generated/src/assets.c
        ${CMAKE_CURRENT_BINARY_DIR}/generated/include/assets.h)

set(PRECOMPILED_HEADERS)

set(HEADERS)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c)

add_header(${CMAKE_CURRENT_SOURCE_DIR}/include/defines.h true)
add_header(${CMAKE_CURRENT_SOURCE_DIR}/include/trigger.h true)
add_header(${CMAKE_CURRENT_SOURCE_DIR}/include/typedefs.h true)
add_header(${CMAKE_CURRENT_SOURCE_DIR}/include/nuklear_default.h true)

add_source(${CMAKE_CURRENT_SOURCE_DIR} maps true)
add_source(${CMAKE_CURRENT_SOURCE_DIR} nk_util_functions true)
add_source(${CMAKE_CURRENT_SOURCE_DIR} int_vec2 true)
add_source(${CMAKE_CURRENT_SOURCE_DIR} ghosts true)
add_source(${CMAKE_CURRENT_SOURCE_DIR} utils true)
add_source(${CMAKE_CURRENT_BINARY_DIR}/generated assets true)

list(APPEND SOURCES ${HEADERS})
message(STATUS "HEADERS: ${HEADERS}")
message(STATUS "SOURCES: ${SOURCES}")

if (EMSCRIPTEN)
    # set(SUPPORT_CLANGD OFF CACHE BOOL "" FORCE)
    set(PLATFORM
            "Web"
            CACHE STRING "" FORCE)
    # remove --closure-args="--compilation_level SIMPLE" when
    # https://github.com/emscripten-core/emscripten/issues/16706 is fixed
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --no-heap-copy")
    set(CMAKE_EXECUTABLE_SUFFIX ".html")

    set(LINK_FLAGS CMAKE_EXE_LINKER_FLAGS)

    set(${LINK_FLAGS} "${${LINK_FLAGS}} -sGL_ENABLE_GET_PROC_ADDRESS")
    set(${LINK_FLAGS} "${${LINK_FLAGS}} -sWASM=1")
    set(${LINK_FLAGS} "${${LINK_FLAGS}} -sMINIFY_HTML=1")
    set(${LINK_FLAGS} "${${LINK_FLAGS}} -sENVIRONMENT=web")
    set(${LINK_FLAGS} "${${LINK_FLAGS}} -sFILESYSTEM=1")
    set(${LINK_FLAGS} "${${LINK_FLAGS}} -sALLOW_MEMORY_GROWTH=1")
    set(${LINK_FLAGS}
            "${${LINK_FLAGS}} --pre-js ${CMAKE_CURRENT_SOURCE_DIR}/src/wasm-shell/pre.js"
    )
    set(${LINK_FLAGS} "${${LINK_FLAGS}} --closure 0")
    set(${LINK_FLAGS} "${${LINK_FLAGS}} -sMODULARIZE")
    set(${LINK_FLAGS} "${${LINK_FLAGS}} -flto")
    set(${LINK_FLAGS} "${${LINK_FLAGS}} -sEXPORT_NAME=PacMan")
    set(${LINK_FLAGS} "${${LINK_FLAGS}} -sINVOKE_RUN=1")
    # set(${LINK_FLAGS} "${${LINK_FLAGS}} -sWASM_ASYNC_COMPILATION")
    set(${LINK_FLAGS}
            "${${LINK_FLAGS}} --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/src/wasm-shell/shell.html"
    )

    # Remove the -rdynamic flag because otherwise emscripten does not generate
    # HTML+JS+WASM files, only a non-working and fat HTML
    string(REPLACE "-rdynamic" "" CMAKE_SHARED_LIBRARY_LINK_C_FLAGS
            "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS}")
endif ()

file(DOWNLOAD
        https://github.com/cpm-cmake/CPM.cmake/releases/latest/download/CPM.cmake
        ${CMAKE_BINARY_DIR}/CPM.cmake)
include(${CMAKE_BINARY_DIR}/CPM.cmake)

cpmaddpackage("gh:libsdl-org/SDL#main")

cpmaddpackage("gh:libsdl-org/SDL_image#main")

CPMAddPackage(
        NAME CPMLicenses.cmake
        GITHUB_REPOSITORY cpm-cmake/CPMLicenses.cmake
        VERSION 0.0.7
)

cpmaddpackage(NAME nuklear_package GITHUB_REPOSITORY Immediate-Mode-UI/Nuklear GIT_TAG master DOWNLOAD_ONLY YES)

add_library(nuklear INTERFACE)
target_include_directories(nuklear INTERFACE ${nuklear_package_SOURCE_DIR})

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} SDL3::SDL3 SDL3_image::SDL3_image nuklear)
target_compile_definitions(${PROJECT_NAME} PRIVATE NAME="${PROJECT_NAME}")

if (EMSCRIPTEN)
    set_target_properties(
            ${PROJECT_NAME}
            PROPERTIES LINK_DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/src/wasm-shell/shell.html)
    set_target_properties(
            ${PROJECT_NAME}
            PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/wasm-shell/pre.js)
endif ()

target_include_directories(
        ${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/generated/include)

target_precompile_headers(${PROJECT_NAME} PRIVATE ${PRECOMPILED_HEADERS})
target_compile_definitions(${PROJECT_NAME} PRIVATE -DFPS=${PACMAN_FPS})

set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 23)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/create_clangd.cmake)

cpm_licenses_create_disclaimer_target(
        write-licenses "${CMAKE_CURRENT_BINARY_DIR}/third_party.txt" "${CPM_PACKAGES}"
)

# get_all_targets(all_targets ${CMAKE_CURRENT_SOURCE_DIR})
# include(ProcessorCount) ProcessorCount(N) foreach (t ${all_targets})
# get_target_property(t_sources ${t} SOURCES) list(LENGTH t_sources len)
# math(EXPR batch_size "${len} / ${N}" OUTPUT_FORMAT DECIMAL) if (${batch_size}
# LESS 1) set(batch_size 1) endif () message(STATUS "${t}: ${batch_size} =
# ${len} / ${N}") set_target_properties(${t} PROPERTIES UNITY_BUILD_MODE BATCH
# UNITY_BUILD_BATCH_SIZE batch_size) endforeach ()
