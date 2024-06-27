#ifndef TYPEDEFS_H
#define TYPEDEFS_H
#include "SDL3/SDL_render.h"
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_STANDARD_BOOL
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#include "nuklear.h"

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
    float position[2];
    float uv[2];
    nk_byte col[4];
} nk_sdl_vertex_t;

typedef struct
{
    struct
    {
        struct
        {
            SDL_Window *window;
            SDL_Texture *framebuffer_texture;
            SDL_Renderer *renderer;
            struct
            {
                SDL_Texture *font;
                SDL_Texture *atlas;
                atlas_entry_t map_filled;
                atlas_entry_t map;
                atlas_entry_t pacman[5][4];
                atlas_entry_t pacman_die[13];
            } sprites;
            int width;
            int height;
            SDL_FRect target_rect;
            SDL_bool bordered;
        } sdl;
        struct
        {
            struct nk_context ctx;
            struct nk_font_atlas atlas;
            struct nk_draw_null_texture tex_null;
            struct nk_buffer cmds;
        } nuklear;
    } video;
    struct
    {
        int_vec2_t position;
        int position_interp;
        direction_t direction;
        direction_t next_direction;
        uint32_t animation;
        bool dead;
    } pacman;
    cell_type_t map[(int)GRID_HEIGHT * (int)GRID_WIDTH];
    bool running;
    struct
    {
        trigger_t pacman_animation;
        trigger_t pacman_move;
        trigger_t pacman_move_between_cells;
        trigger_t pacman_die;
    } trigger;
    uint32_t last_ticks;
    uint32_t delta;
    uint32_t tick;
} state_t;

#endif // TYPEDEFS_H
