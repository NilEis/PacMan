#define SDL_MAIN_USE_CALLBACKS
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_main.h"
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

#include <SDL3/SDL_render.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

bool event_key (const SDL_Event *event, state_t *state);

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

int SDL_AppInit (void **appstate, int argc, char **argv)
{
    (void)argc;
    (void)argv;
    state_t *state = calloc (1, sizeof (state_t));
    memccpy (state->map, );
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
    state->sdl.window = SDL_CreateWindow (
        NAME, WIDTH, HEIGHT, SDL_WINDOW_BORDERLESS | SDL_WINDOW_TRANSPARENT);
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
    {
        SDL_IOStream *file = SDL_IOFromConstMem (
            asset_general_sprites_png, asset_general_sprites_png_size);
        state->sdl.sprites.atlas
            = IMG_LoadTexture_IO (state->sdl.renderer, file, SDL_TRUE);
    }
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
    switch (event->key.keysym.scancode)
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
    }
    return false;
}
