#define UTILS_PRIVATE_FUNCS
#include "utils.h"
#include "SDL3_image/SDL_image.h"
#include "assets.h"
#include <stdio.h>

static cell_type_t test_cell (
    const SDL_Surface *map_surface, const int y, const int x)
{
    for (auto yp = 0; yp < TILE_HEIGHT; yp++)
    {
        for (auto xp = 0; xp < TILE_WIDTH; xp++)
        {
            uint8_t r = 1;
            uint8_t g = 2;
            uint8_t b = 3;
            const auto format_details
                = SDL_GetPixelFormatDetails (map_surface->format);
            const auto pixel
                = *(uint32_t *)((uint8_t *)map_surface->pixels
                                + (y * TILE_HEIGHT + yp) * map_surface->pitch
                                + (x * TILE_WIDTH + xp)
                                      * format_details->bytes_per_pixel);
            SDL_GetRGB (pixel, format_details, nullptr, &r, &g, &b);
            __attribute__ ((unused)) const auto i
                = (int)r << 16 | (int)g << 8 | b;
            if (r == 255)
            {
                return CELL_GHOST_WALL;
            }
            if (b == 255)
            {
                return CELL_WALL;
            }
        }
    }
    return CELL_EMPTY;
}

bool try_move (const state_t *const state,
    int *x,
    int *y,
    const direction_t direction,
    const bool is_ghost)
{
    int cx = 0;
    int cy = 0;
    bool res = false;
    if (try_move_and_set (state, *x, *y, &cx, &cy, direction, is_ghost))
    {
        *x = cx;
        *y = cy;
        res = true;
    }
    return res;
}

bool try_move_and_set (const state_t *const state,
    const int x,
    const int y,
    int *out_x,
    int *out_y,
    const direction_t direction,
    const bool is_ghost)
{
    auto cx = x;
    auto cy = y;
    bool res = false;
    switch (direction)
    {
    case DIRECTION_RIGHT:
        cx++;
        break;
    case DIRECTION_LEFT:
        cx--;
        break;
    case DIRECTION_UP:
        cy--;
        break;
    case DIRECTION_DOWN:
        cy++;
        break;
    default:
        return false;
    }
    cx = (int)((cx + (uint32_t)GRID_WIDTH) % (uint32_t)GRID_WIDTH);
    cy = (int)((cy + (uint32_t)GRID_HEIGHT) % (uint32_t)GRID_HEIGHT);
    *out_x = cx;
    *out_y = cy;
    if (state->map[coords_to_map_index (cy, cx)] != CELL_WALL
        && (is_ghost
            || state->map[coords_to_map_index (cy, cx)] != CELL_GHOST_WALL))
    {
        res = true;
    }
    return res;
}

void parse_map (state_t *state, const bool load_atlas)
{
    SDL_IOStream *file = SDL_IOFromConstMem (
        asset_general_sprites_png, asset_general_sprites_png_size);
    SDL_Surface *full_asset_image = IMG_LoadTyped_IO (file, true, "PNG");
    if (load_atlas)
        state->video.sdl.sprites.atlas = SDL_CreateTextureFromSurface (
            state->video.sdl.renderer, full_asset_image);
    SDL_Surface *map_surface
        = SDL_CreateSurface ((int)state->video.sdl.sprites.map.pos.w,
            (int)state->video.sdl.sprites.map.pos.h,
            SDL_PIXELFORMAT_RGBA32);
    const auto res = SDL_BlitSurface (full_asset_image,
        &(SDL_Rect){ (int)state->video.sdl.sprites.map.pos.x,
            (int)state->video.sdl.sprites.map.pos.y,
            (int)state->video.sdl.sprites.map.pos.w,
            (int)state->video.sdl.sprites.map.pos.h },
        map_surface,
        NULL);
    if (res != 0)
    {
        printf ("%d: %s\n", res, SDL_GetError ());
    }
    for (auto y = 0; y < GRID_HEIGHT; y++)
    {
        for (auto x = 0; x < GRID_WIDTH; x++)
        {
            state->map[coords_to_map_index (y, x)]
                = test_cell (map_surface, y, x);
        }
    }
    SDL_DestroySurface (full_asset_image);
    SDL_DestroySurface (map_surface);
}