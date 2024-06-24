#include "maps.h"
#define SDL_MAIN_USE_CALLBACKS
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_main.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_timer.h"
#include "SDL3_image/SDL_image.h"
#include "assets.h"
#include "defines.h"
#include "trigger.h"
#include "typedefs.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

bool event_key (const SDL_Event *event, state_t *state);

int SDL_AppInit (void **appstate, int argc, char **argv)
{
    (void)argc;
    (void)argv;
    state_t *state = calloc (1, sizeof (state_t));
    memcpy (state->map, map1, sizeof (state->map));
    state->pacman.position.x = 1;
    state->pacman.position.y = 1;
    state->pacman.direction = DIRECTION_LEFT;
    state->pacman.next_direction = DIRECTION_NONE;
    state->pacman.animation = 0;

    trigger_stop (&state->trigger.pacman_animation);
    trigger_stop (&state->trigger.pacman_move);

    *appstate = state;
    const auto result = SDL_InitSubSystem (SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    if (result < 0)
    {
        goto error;
    }
    SDL_SetHint ("SDL_RENDER_SCALE_QUALITY", "0");
    state->sdl.window = SDL_CreateWindow (NAME,
        WIDTH,
        HEIGHT,
        SDL_WINDOW_BORDERLESS | SDL_WINDOW_TRANSPARENT | SDL_WINDOW_RESIZABLE);
    SDL_SetWindowSurfaceVSync (state->sdl.window, 1);
    state->sdl.bordered = false;
    state->sdl.width = WIDTH;
    state->sdl.width = HEIGHT;
    if (state->sdl.window == NULL)
    {
        goto error;
    }
    state->sdl.renderer = SDL_CreateRenderer (state->sdl.window, NULL);
    state->sdl.framebuffer_texture = SDL_CreateTexture (state->sdl.renderer,
        SDL_PIXELFORMAT_RGBA5551,
        SDL_TEXTUREACCESS_TARGET,
        TEXTURE_WIDTH,
        TEXTURE_HEIGHT);

    SDL_SetRenderLogicalPresentation (state->sdl.renderer,
        WIDTH,
        HEIGHT,
        SDL_LOGICAL_PRESENTATION_INTEGER_SCALE,
        SDL_SCALEMODE_NEAREST);
    {
        state->sdl.sprites.map_filled.pos
            = (SDL_FRect){ .x = 0.0, .y = 0.0, .w = 224.0, .h = 248.0 };
        state->sdl.sprites.map.pos
            = (SDL_FRect){ .x = 228.0, .y = 0.0, .w = 224.0, .h = 248.0 };

        state->sdl.sprites.pacman[DIRECTION_RIGHT][2].pos
            = state->sdl.sprites.pacman[DIRECTION_LEFT][2].pos
            = state->sdl.sprites.pacman[DIRECTION_UP][2].pos
            = state->sdl.sprites.pacman[DIRECTION_DOWN][2].pos
            = state->sdl.sprites.pacman[DIRECTION_NONE][0].pos
            = state->sdl.sprites.pacman[DIRECTION_NONE][1].pos
            = state->sdl.sprites.pacman[DIRECTION_NONE][2].pos
            = state->sdl.sprites.pacman[DIRECTION_NONE][3].pos
            = (SDL_FRect){ .x = 488.0, .y = 0.0, .w = 15.0, .h = 15.0 };

        state->sdl.sprites.pacman[DIRECTION_RIGHT][0].pos
            = (SDL_FRect){ .x = 456.0, .y = 0.0, .w = 15.0, .h = 15.0 };
        state->sdl.sprites.pacman[DIRECTION_RIGHT][1].pos
            = (SDL_FRect){ .x = 472.0, .y = 0.0, .w = 15.0, .h = 15.0 };
        state->sdl.sprites.pacman[DIRECTION_RIGHT][3].pos
            = (SDL_FRect){ .x = 472.0, .y = 0.0, .w = 15.0, .h = 15.0 };

        state->sdl.sprites.pacman[DIRECTION_LEFT][0].pos
            = (SDL_FRect){ .x = 456.0, .y = 16.0, .w = 15.0, .h = 15.0 };
        state->sdl.sprites.pacman[DIRECTION_LEFT][1].pos
            = (SDL_FRect){ .x = 472.0, .y = 16.0, .w = 15.0, .h = 15.0 };
        state->sdl.sprites.pacman[DIRECTION_LEFT][3].pos
            = (SDL_FRect){ .x = 472.0, .y = 16.0, .w = 15.0, .h = 15.0 };

        state->sdl.sprites.pacman[DIRECTION_UP][0].pos
            = (SDL_FRect){ .x = 456.0, .y = 32.0, .w = 15.0, .h = 15.0 };
        state->sdl.sprites.pacman[DIRECTION_UP][1].pos
            = (SDL_FRect){ .x = 472.0, .y = 32.0, .w = 15.0, .h = 15.0 };
        state->sdl.sprites.pacman[DIRECTION_UP][3].pos
            = (SDL_FRect){ .x = 472.0, .y = 32.0, .w = 15.0, .h = 15.0 };

        state->sdl.sprites.pacman[DIRECTION_DOWN][0].pos
            = (SDL_FRect){ .x = 456.0, .y = 48.0, .w = 15.0, .h = 15.0 };
        state->sdl.sprites.pacman[DIRECTION_DOWN][1].pos
            = (SDL_FRect){ .x = 472.0, .y = 48.0, .w = 15.0, .h = 15.0 };
        state->sdl.sprites.pacman[DIRECTION_DOWN][3].pos
            = (SDL_FRect){ .x = 472.0, .y = 48.0, .w = 15.0, .h = 15.0 };
    }

    {
        SDL_IOStream *file = SDL_IOFromConstMem (
            asset_general_sprites_png, asset_general_sprites_png_size);
        SDL_Surface *full_asset_image
            = IMG_LoadTyped_IO (file, SDL_TRUE, "PNG");
        state->sdl.sprites.atlas = SDL_CreateTextureFromSurface (
            state->sdl.renderer, full_asset_image);
        SDL_Surface *map_surface
            = SDL_CreateSurface (state->sdl.sprites.map.pos.w,
                state->sdl.sprites.map.pos.h,
                SDL_PIXELFORMAT_RGBA32);
        const auto res = SDL_BlitSurface (full_asset_image,
            &(SDL_Rect){ state->sdl.sprites.map.pos.x,
                state->sdl.sprites.map.pos.y,
                state->sdl.sprites.map.pos.w,
                state->sdl.sprites.map.pos.h },
            map_surface,
            NULL);
        if (res != 0)
        {
            printf ("%d: %s\n", res, SDL_GetError ());
        }
        for (auto y = 0; y < GRID_HEIGHT; y++)
        {
            for (auto x = 0; x < GRID_HEIGHT; x++)
            {
                uint32_t avg_pixel = 0;
                for (auto yp = 0; yp < TILE_HEIGHT; yp++)
                {
                    for (auto xp = 0; xp < TILE_WIDTH; xp++)
                    {
                        uint8_t r = 1;
                        uint8_t g = 2;
                        uint8_t b = 3;
                        const auto pixel
                            = *(uint32_t *)((uint8_t *)map_surface->pixels
                                            + (y * TILE_HEIGHT + yp)
                                                  * map_surface->pitch
                                            + (x * TILE_WIDTH + xp)
                                                  * map_surface->format
                                                        ->bytes_per_pixel);
                        SDL_GetRGB (pixel, map_surface->format, &r, &g, &b);
                        const auto i = (int)r << 16 | (int)g << 8 | b;
                        avg_pixel += i;
                        if (b != 0)
                        {
                            state->map[y * (int)GRID_WIDTH + x] = CELL_WALL;
                            goto external_loop;
                        }
                    }
                }
external_loop:
            }
        }
        SDL_DestroySurface (full_asset_image);
        SDL_DestroySurface (map_surface);
    }

    state->running = true;
    state->last_ticks = SDL_GetTicks ();
    trigger_start_after (
        state, &state->trigger.pacman_animation, PACMAN_ANIMATION_TICKS);
    trigger_start_after (
        state, &state->trigger.pacman_move, PACMAN_MOVE_TICKS);
    return 0;

error:
    return -1;
}

int SDL_AppIterate (void *appstate)
{
    const auto state = (state_t *)appstate;
    const auto delta = SDL_GetTicks () - state->last_ticks;
    state->delta = delta;
    state->last_ticks = SDL_GetTicks ();

    if (trigger_triggered (state, &state->trigger.pacman_animation))
    {
        state->pacman.animation = (state->pacman.animation + 1) % 4;
        trigger_start_after (
            state, &state->trigger.pacman_animation, PACMAN_ANIMATION_TICKS);
    }
    if (trigger_triggered (state, &state->trigger.pacman_move))
    {
        auto x = state->pacman.position.x;
        auto y = state->pacman.position.y;
        switch (state->pacman.direction)
        {
        case DIRECTION_RIGHT:
            x++;
            break;
        case DIRECTION_LEFT:
            x--;
            break;
        case DIRECTION_UP:
            y--;
            break;
        case DIRECTION_DOWN:
            y++;
            break;
        default:
            break;
        }
        x = (x + (uint32_t)GRID_WIDTH) % (uint32_t)GRID_WIDTH;
        y = (y + (uint32_t)GRID_HEIGHT) % (uint32_t)GRID_HEIGHT;
        if (state->map[y * (int)GRID_WIDTH + x] != CELL_WALL
            && state->map[y * (int)GRID_WIDTH + x] != CELL_GHOST_WALL)
        {
            state->pacman.position.x = x;
            state->pacman.position.y = y;
        }
        trigger_start_after (
            state, &state->trigger.pacman_move, PACMAN_MOVE_TICKS);
    }

    SDL_SetRenderTarget (state->sdl.renderer, state->sdl.framebuffer_texture);
    {
        SDL_SetRenderDrawColor (state->sdl.renderer, 0, 0, 0, 0);
        SDL_RenderClear (state->sdl.renderer);
        SDL_RenderTexture (state->sdl.renderer,
            state->sdl.sprites.atlas,
            &state->sdl.sprites.map.pos,
            NULL);
        for (auto y = 0; y < GRID_HEIGHT; y++)
        {
            for (auto x = 0; x < GRID_HEIGHT; x++)
            {
                SDL_SetRenderDrawColor (state->sdl.renderer,
                    0,
                    0,
                    (state->map[y * (int)GRID_WIDTH + x] == CELL_WALL) * 255,
                    (state->map[y * (int)GRID_WIDTH + x] == CELL_WALL) * 255);
                SDL_RenderFillRect (state->sdl.renderer,
                    &(SDL_FRect){ x * CELL_WIDTH,
                        y * CELL_HEIGHT,
                        CELL_WIDTH,
                        CELL_HEIGHT });
            }
        }
        SDL_RenderTexture (state->sdl.renderer,
            state->sdl.sprites.atlas,
            &state->sdl.sprites
                 .pacman[state->pacman.direction][state->pacman.animation]
                 .pos,
            &(SDL_FRect){ state->pacman.position.x * CELL_WIDTH,
                state->pacman.position.y * CELL_HEIGHT,
                CELL_WIDTH,
                CELL_HEIGHT });
    }
    SDL_SetRenderTarget (state->sdl.renderer, NULL);
    {
        SDL_RenderTexture (
            state->sdl.renderer, state->sdl.framebuffer_texture, NULL, NULL);
    }
    SDL_RenderPresent (state->sdl.renderer);
    while (SDL_GetTicks () - state->last_ticks < TICK_SPEED)
    {
    }
    return SDL_APP_CONTINUE;
}

int SDL_AppEvent (void *appstate, const SDL_Event *event)
{
    state_t *state = appstate;
    switch (event->type)
    {
    case SDL_EVENT_QUIT:
        state->running = false;
        return SDL_APP_SUCCESS;
    case SDL_EVENT_KEY_DOWN:
        if (event_key (event, state))
            return SDL_APP_SUCCESS;
        break;
    default:
        break;
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit (void *appstate)
{
    state_t *state = appstate;
    SDL_DestroyTexture (state->sdl.framebuffer_texture);
    SDL_DestroyTexture (state->sdl.sprites.atlas);
    SDL_DestroyRenderer (state->sdl.renderer);
    SDL_DestroyWindow (state->sdl.window);
    free (state);
}

bool event_key (const SDL_Event *event, state_t *state)
{
    switch (event->key.scancode)
    {
    case SDL_SCANCODE_ESCAPE:
        return true;
    case SDL_SCANCODE_LEFT:
        state->pacman.direction = DIRECTION_LEFT;
        break;
    case SDL_SCANCODE_RIGHT:
        state->pacman.direction = DIRECTION_RIGHT;
        break;
    case SDL_SCANCODE_UP:
        state->pacman.direction = DIRECTION_UP;
        break;
    case SDL_SCANCODE_DOWN:
        state->pacman.direction = DIRECTION_DOWN;
        break;
    case SDL_SCANCODE_F:
    {
        const auto fps = 1000 / state->delta;
        printf ("fps: %u\n", fps);
    }
    break;
    case SDL_SCANCODE_H:
        state->sdl.bordered = !state->sdl.bordered;
        SDL_SetWindowBordered (state->sdl.window, state->sdl.bordered);
        SDL_SetWindowResizable (state->sdl.window, state->sdl.bordered);
    }
    return false;
}
