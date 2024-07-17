#ifndef DEFINES_H
#define DEFINES_H
#include <stdio.h>

#define STDOUT_BUF_SIZE (BUFSIZ)
#if defined(__unix__) || defined(__EMSCRIPTEN__)
#define STDOUT_REOPEN_NAME "/dev/null"
#elif defined(_WIN32) || defined(WIN32)
#define STDOUT_REOPEN_NAME "nul"
#else
#error "unknown OS"
#endif

#define PACMAN_MIN(a, b) ((a) < (b) ? (a) : (b))
#define PACMAN_MAX(a, b) ((a) > (b) ? (a) : (b))

#define TILE_WIDTH (8)
#define TILE_HEIGHT (8)

#define GRID_WIDTH (28.0)
#define GRID_HEIGHT (31.0)

#define GRID_RATIO (GRID_WIDTH / GRID_HEIGHT)
#define GRID_RATIO_INV (1.0 / (GRID_WIDTH / GRID_HEIGHT))

#ifndef FPS
#define FPS (60.0)
#endif

#define PACMAN_GAME_SPEED (1.0)
#define MS_PER_FRAME (1000.0f / (double)FPS)
#define MS_TO_TICKS(ms)                                                       \
    PACMAN_MAX (1, (double)(ms) / (MS_PER_FRAME * PACMAN_GAME_SPEED))

#define PACMAN_ANIMATION_TICKS (MS_TO_TICKS (64))
#define PACMAN_MOVE_TICKS (MS_TO_TICKS (160))
#define PACMAN_MOVE_BETWEEN_CELLS_TICKS (1.0)

#define GHOST_ANIMATION_TICKS (MS_TO_TICKS (140))
#define GHOST_MOVE_TICKS (PACMAN_MOVE_TICKS * 2)
#define GHOST_MOVE_BETWEEN_CELLS_TICKS (1.0)

#define CELL_WIDTH (TILE_WIDTH * 2.5)
#define CELL_HEIGHT (TILE_HEIGHT * 2.5)

#ifndef WIDTH
#define WIDTH (CELL_WIDTH * GRID_WIDTH)
#endif

#ifndef HEIGHT
#define HEIGHT (CELL_HEIGHT * GRID_HEIGHT)
#endif

#define TEXTURE_WIDTH (WIDTH)
#define TEXTURE_HEIGHT (HEIGHT)

#ifndef NAME
#define NAME "CMAKE_SHOULD_SET_NAME"
#endif

#endif /* DEFINES_H */
