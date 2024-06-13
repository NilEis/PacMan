cmake_minimum_required(VERSION 3.19) # Set the minimum required CMake version
project(fedit C)

option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/options.cmake)

set(CMAKE_C_STANDARD 23)
set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c)

if(EMSCRIPTEN)
  set(SUPPORT_CLANGD
      OFF
      CACHE BOOL "" FORCE)
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
  set(${LINK_FLAGS} "${${LINK_FLAGS}} -sUSE_GLFW=3")
  set(${LINK_FLAGS} "${${LINK_FLAGS}} -sWASM=1")
  set(${LINK_FLAGS} "${${LINK_FLAGS}} -sMINIFY_HTML=1")
  set(${LINK_FLAGS} "${${LINK_FLAGS}} -sENVIRONMENT=web")
  set(${LINK_FLAGS} "${${LINK_FLAGS}} -sFILESYSTEM=0")
  set(${LINK_FLAGS} "${${LINK_FLAGS}} -sALLOW_MEMORY_GROWTH=1")
  set(${LINK_FLAGS}
      "${${LINK_FLAGS}} --pre-js ${CMAKE_CURRENT_SOURCE_DIR}/src/wasm-shell/pre.js"
  )
  set(${LINK_FLAGS} "${${LINK_FLAGS}} -sASYNCIFY")
  set(${LINK_FLAGS} "${${LINK_FLAGS}} --closure 1")
  set(${LINK_FLAGS} "${${LINK_FLAGS}} -sMODULARIZE")
  set(${LINK_FLAGS} "${${LINK_FLAGS}} -flto")
  set(${LINK_FLAGS} "${${LINK_FLAGS}} -sEXPORT_NAME=PacMan")
  set(${LINK_FLAGS} "${${LINK_FLAGS}} -sWASM_ASYNC_COMPILATION")
  set(${LINK_FLAGS}
      "${${LINK_FLAGS}} --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/src/wasm-shell/shell.html"
  )

  # Remove the -rdynamic flag because otherwise emscripten does not generate
  # HTML+JS+WASM files, only a non-working and fat HTML
  string(REPLACE "-rdynamic" "" CMAKE_SHARED_LIBRARY_LINK_C_FLAGS
                 "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS}")
endif()

include(FetchContent)

FetchContent_Declare(raylib
                     GIT_REPOSITORY https://github.com/raysan5/raylib.git)

set(BUILD_EXAMPLES
    OFF
    CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} raylib)

if(EMSCRIPTEN)
  set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES LINK_DEPENDS
               ${CMAKE_CURRENT_SOURCE_DIR}/src/wasm-shell/shell.html)
  set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/wasm-shell/pre.js)
endif()

target_include_directories(${PROJECT_NAME}
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

option(SUPPORT_CLANGD "Support clangd" ON)
if(SUPPORT_CLANGD)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  set(CLANGD_SRC ${CMAKE_CURRENT_SOURCE_DIR}/cmake/.clangd.in)
  set(CLANGD_DEST ${CMAKE_CURRENT_SOURCE_DIR}/.clangd)
  configure_file(${CLANGD_SRC} ${CLANGD_DEST} @ONLY)
endif()
