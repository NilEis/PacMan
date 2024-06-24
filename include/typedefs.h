#ifndef TYPEDEFS_H
#define TYPEDEFS_H
#include "SDL3/SDL_render.h"

#include <stdint.h>

typedef enum
{
    DIRECTION_RIGHT = 0,
    DIRECTION_LEFT = 1,
    DIRECTION_UP = 2,
    DIRECTION_DOWN = 3,
    DIRECTION_NONE = 4
} direction_t;

typedef enum
{
    CELL_EMPTY,
    CELL_WALL,
    CELL_COIN,
    CELL_GHOST_WALL
} cell_type_t;

typedef struct
{
    int x;
    int y;
} int_vec2_t;

typedef struct
{
    int_vec2_t pos;
    int_vec2_t goal;
    float t;
} movement_t;

typedef struct
{
    uint32_t tick;
} trigger_t;

typedef struct
{
    SDL_FRect pos;
} atlas_entry_t;

typedef struct
{
    struct
    {
        SDL_Window *window;
        SDL_Texture *framebuffer_texture;
        SDL_Renderer *renderer;
        struct
        {
            SDL_Texture *atlas;
            atlas_entry_t map_filled;
            atlas_entry_t map;
            atlas_entry_t pacman[5][4];
        } sprites;
        int width;
        int height;
        SDL_bool bordered;
    } sdl;
    struct
    {
        int_vec2_t position;
        direction_t direction;
        direction_t next_direction;
        uint32_t animation;
    } pacman;
    cell_type_t map[(int)GRID_HEIGHT * (int)GRID_WIDTH];
    bool running;
    struct
    {
        trigger_t pacman_animation;
        trigger_t pacman_move;
    } trigger;
    uint32_t last_ticks;
    uint32_t delta;
} state_t;

#endif // TYPEDEFS_H
