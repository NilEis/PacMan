set(SDL_DIALOG
        OFF
        CACHE BOOL "" FORCE)

set(SDL_STATIC
        ON
        CACHE BOOL "" FORCE)

set(SDL_TEST_LIBRARY
        OFF
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_WERROR
        OFF
        CACHE BOOL "" FORCE)
set(SDL_WERROR
        OFF
        CACHE BOOL "" FORCE)

set(PACMAN_FPS
        60
        CACHE STRING "the fps of pacman")

set(SDL3IMAGE_AVIF
        OFF
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_AVIF_SAVE
        OFF
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_AVIF_SHARED
        OFF
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_BACKEND_STB
        ON
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_BACKEND_WIC
        OFF
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_BMP
        OFF
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_DEPS_SHARED
        ON
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_GIF
        OFF
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_JPG
        OFF
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_JPG_SAVE
        OFF
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_JXL
        OFF
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_LBM
        OFF
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_PCX
        OFF
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_PNG
        ON
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_PNG_SAVE
        ON
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_PNM
        OFF
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_QOI
        OFF
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_SAMPLES
        OFF
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_STRICT
        OFF
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_SVG
        OFF
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_TESTS
        OFF
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_TESTS_INSTALL
        OFF
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_TGA
        OFF
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_TIF
        OFF
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_TIF_SHARED
        OFF
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_VENDORED
        OFF
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_WEBP
        OFF
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_WEBP_SHARED
        OFF
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_WERROR
        OFF
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_XCF
        OFF
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_XPM
        OFF
        CACHE BOOL "" FORCE)
set(SDL3IMAGE_XV
        OFF
        CACHE BOOL "" FORCE)

if (NOT EMSCRIPTEN)
    option(REROUTE_STDOUT "Output stdout in a window" ON)
endif (NOT EMSCRIPTEN)
