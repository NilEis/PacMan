#ifndef TYPEDEFS_H
#define TYPEDEFS_H
#include "SDL3/SDL_render.h"
#include "defines.h"
#include "nuklear_default.h"

#include <stdint.h>

typedef enum
{
    DIRECTION_RIGHT = 0,
    DIRECTION_LEFT = 1,
    DIRECTION_UP = 2,
    DIRECTION_DOWN = 3,
    DIRECTION_NONE = 4
} direction_t;

#include "int_vec2.h"

typedef enum
{
    CELL_EMPTY,
    CELL_WALL,
    CELL_COIN,
    CELL_GHOST_WALL
} cell_type_t;

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

typedef enum : uint8_t
{
    GHOST_BLINKY = 0,
    GHOST_PINKY = 1,
    GHOST_INKY = 2,
    GHOST_CLYDE = 3,
    GHOST_MAX
} ghost_name_t;

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_t;

typedef struct
{
    int_vec2_t pos;
    int position_interp;
    int_vec2_t corner;
    int_vec2_t target;
    direction_t dir;
    struct
    {
        struct
        {
            trigger_t trigger;
            uint32_t ticks;
        } start;
        struct
        {
            trigger_t trigger;
            uint32_t ticks;
        } move;
        trigger_t move_between;
        trigger_t chase;
        trigger_t scatter;
    } trigger;
    SDL_FRect sprite[4][2];
    rgb_t color;
} ghost_t;

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
        struct
        {
            trigger_t pacman_animation;
            trigger_t pacman_move;
            trigger_t pacman_move_between_cells;
            trigger_t pacman_die;
        } trigger;
        int_vec2_t position;
        int position_interp;
        direction_t direction;
        direction_t next_direction;
        uint32_t animation;
        bool dead;
    } pacman;
    struct
    {
        ghost_t ghost[4];
        struct
        {
            trigger_t trigger;
            uint32_t value;
        } animation;
    } ghosts;
    cell_type_t map[(int)GRID_HEIGHT * (int)GRID_WIDTH];
    bool running;
    uint64_t last_ticks;
    uint32_t delta;
    uint32_t tick;
    struct
    {
        nk_bool draw_targets;
        nk_bool draw_ghost_cell;
        nk_bool draw_pacman_cell;
        nk_bool pause;
    } options;
    FILE *buffer;
} state_t;

#endif // TYPEDEFS_H
