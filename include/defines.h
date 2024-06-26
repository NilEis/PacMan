#ifndef DEFINES_H
#define DEFINES_H

#define TICK_SPEED 16 /* in ms */

#define TILE_WIDTH (8)
#define TILE_HEIGHT (8)

#define SPRITE_WIDTH (16)
#define SPRITE_HEIGHT (16)

#define GRID_WIDTH (28.0)
#define GRID_HEIGHT (31.0)

#define GRID_RATIO (GRID_WIDTH / GRID_HEIGHT)
#define GRID_RATIO_INV (1.0 / (GRID_WIDTH / GRID_HEIGHT))

#define FPS (60.0)
#define MS_PER_FRAME (1000.0 / FPS)
#define MS_TO_TICKS(ms) ((double)(ms) / MS_PER_FRAME)

#define PACMAN_ANIMATION_TICKS (MS_TO_TICKS (64))
#define PACMAN_MOVE_TICKS (MS_TO_TICKS (160))
#define PACMAN_MOVE_BETWEEN_CELLS_TICKS (1.0)

#define CELL_WIDTH (TILE_WIDTH * 2)
#define CELL_HEIGHT (TILE_HEIGHT * 2)

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
