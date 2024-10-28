#include "maps.h"
#define SDL_MAIN_USE_CALLBACKS
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_main.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_scancode.h"
#include "SDL3/SDL_timer.h"
#include "SDL3_image/SDL_image.h"
#include "defines.h"
#include "ghosts.h"
#include "int_vec2.h"
#include "nk_util_functions.h"
#include "trigger.h"
#include "typedefs.h"
#define UTILS_PRIVATE_FUNCS
#include "utils.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool event_key (const SDL_Event *event, state_t *state);
static void resize_event (state_t *state, int width, int height);

static void init_state (state_t *state,
    const int init_width,
    const int init_height,
    const bool load_atlas)
{
    state->pacman.position.x = 1;
    state->pacman.position.y = 1;
    state->pacman.position_interp = PACMAN_MOVE_TICKS;
    state->pacman.dead = false;
    state->pacman.direction = DIRECTION_LEFT;
    state->pacman.next_direction = DIRECTION_NONE;
    state->pacman.animation = 0;
    memcpy (state->map, map1, sizeof (state->map));
    parse_map (state, load_atlas);

    trigger_stop (&state->pacman.trigger.pacman_animation);
    trigger_stop (&state->pacman.trigger.pacman_move);
    trigger_stop (&state->pacman.trigger.pacman_move_between_cells);
    trigger_stop (&state->pacman.trigger.pacman_die);

    trigger_stop (&state->ghosts.animation.trigger);

    for (auto i = 0; i < 4; i++)
    {
        state->ghosts.ghost[i].pos = (int_vec2_t){ .x = 12 + i, .y = 14 };
        state->ghosts.ghost[i].position_interp = GHOST_MOVE_TICKS;
        state->ghosts.ghost[i].trigger.move.ticks = GHOST_MOVE_TICKS;
        state->ghosts.ghost[i].start_path.hit = false;
        state->ghosts.ghost[i].start_path.target = (int_vec2_t){ 15 - i, 11 };
        trigger_stop (&state->ghosts.ghost[i].trigger.move.trigger);
        trigger_stop (&state->ghosts.ghost[i].trigger.move_between);
    }

    state->ghosts.ghost[GHOST_BLINKY].corner
        = (int_vec2_t){ .x = GRID_WIDTH - 3, .y = -4 };
    state->ghosts.ghost[GHOST_PINKY].corner = (int_vec2_t){ .x = 2, .y = -4 };
    state->ghosts.ghost[GHOST_INKY].corner
        = (int_vec2_t){ .x = GRID_WIDTH - 1, .y = GRID_HEIGHT };
    state->ghosts.ghost[GHOST_CLYDE].corner
        = (int_vec2_t){ .x = 0, .y = GRID_HEIGHT };

    state->ghosts.ghost[GHOST_BLINKY].color
        = (rgb_t){ .r = 255, .g = 0, .b = 0 };
    state->ghosts.ghost[GHOST_PINKY].color
        = (rgb_t){ .r = 255, .g = 125, .b = 125 };
    state->ghosts.ghost[GHOST_INKY].color
        = (rgb_t){ .r = 125, .g = 125, .b = 255 };
    state->ghosts.ghost[GHOST_CLYDE].color
        = (rgb_t){ .r = 255, .g = 165, .b = 0 };

    state->video.sdl.bordered = false;
    state->video.sdl.width = init_width;
    state->video.sdl.width = init_height;
    resize_event (state, init_width, init_height);

    state->running = true;
    state->last_ticks = SDL_GetTicks ();
    trigger_start_after (state,
        &state->pacman.trigger.pacman_animation,
        PACMAN_ANIMATION_TICKS);
    trigger_start_after (
        state, &state->pacman.trigger.pacman_move, PACMAN_MOVE_TICKS);
    trigger_start_after (state,
        &state->pacman.trigger.pacman_move_between_cells,
        PACMAN_MOVE_BETWEEN_CELLS_TICKS);

    trigger_start_after (
        state, &state->ghosts.animation.trigger, GHOST_ANIMATION_TICKS);

    for (auto i = 0; i < 4; i++)
    {
        trigger_start_after (state,
            &state->ghosts.ghost[i].trigger.move.trigger,
            state->ghosts.ghost[i].trigger.move.ticks);
        trigger_start_after (state,
            &state->ghosts.ghost[i].trigger.move_between,
            GHOST_MOVE_BETWEEN_CELLS_TICKS);
    }
}

SDL_AppResult SDL_AppInit (void **appstate, const int argc, char **argv)
{
    state_t *state = calloc (1, sizeof (state_t));
#ifdef REROUTE_STDOUT
    memset (state->buffer, 0, STDOUT_BUF_SIZE);
    if (freopen (STDOUT_REOPEN_NAME, "a", stdout) == nullptr)
    {
        printf ("Could not reopen stdout\n");
        goto error;
    }
    if (setvbuf (stdout, state->buffer, _IOFBF, STDOUT_BUF_SIZE))
    {
        perror ("setvbuf");
        goto error;
    }
#endif /* REROUTE_STDOUT */
    printf ("stdout buf size: %d\n", STDOUT_BUF_SIZE);
    printf ("arguments:\n");
    for (auto i = 0; i < argc; i++)
    {
        printf ("    %d: %s\n", i, argv[i]);
    }

    printf ("FPS: %d\n", FPS);

    *appstate = state;
    const auto result = SDL_InitSubSystem (SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    if (result == false)
    {
        goto error;
    }
    SDL_SetHint ("SDL_RENDER_SCALE_QUALITY", "0");
    const SDL_DisplayMode *dm = SDL_GetDesktopDisplayMode (1);
    if (dm == nullptr)
    {
        SDL_Log ("SDL_GetDesktopDisplayMode failed: %s", SDL_GetError ());
    }
    const int init_width = argc < 2 ? dm ? dm->w : WIDTH : atoi (argv[1]);
    const int init_height = argc < 2 ? dm ? dm->h : HEIGHT : atoi (argv[2]);
    // printf ("w: %s - %d\n", argv[1], init_width);
    // printf ("h: %s - %d\n", argv[2], init_height);
    state->video.sdl.window = SDL_CreateWindow (NAME,
        init_width,
        init_height,
        SDL_WINDOW_BORDERLESS | SDL_WINDOW_TRANSPARENT | SDL_WINDOW_RESIZABLE);
    if (state->video.sdl.window == NULL)
    {
        goto error;
    }

    SDL_SetWindowFullscreen (state->video.sdl.window, true);
    state->video.sdl.renderer
        = SDL_CreateRenderer (state->video.sdl.window, nullptr);
    if (state->video.sdl.renderer == nullptr)
    {
        SDL_Log ("SDL_GetDesktopDisplayMode failed: %s", SDL_GetError ());
        goto error;
    }
    SDL_SetRenderDrawBlendMode (
        state->video.sdl.renderer, SDL_BLENDMODE_BLEND);
    printf ("driver:\n");
    for (auto i = 0; i < SDL_GetNumRenderDrivers (); i++)
    {
        if (strcmp (SDL_GetRenderDriver (i),
                SDL_GetRendererName (state->video.sdl.renderer))
            == 0)
        {
            printf (" * ");
        }
        else
        {
            printf ("   ");
        }
        printf ("- %s\n", SDL_GetRenderDriver (i));
    }
    state->video.sdl.framebuffer_texture
        = SDL_CreateTexture (state->video.sdl.renderer,
            SDL_PIXELFORMAT_RGBA5551,
            SDL_TEXTUREACCESS_TARGET,
            TEXTURE_WIDTH,
            TEXTURE_HEIGHT);

    {
        state->video.sdl.sprites.map_filled.pos
            = (SDL_FRect){ .x = 0.f, .y = 0.f, .w = 224.f, .h = 248.f };
        state->video.sdl.sprites.map.pos
            = (SDL_FRect){ .x = 228.f, .y = 0.f, .w = 224.f, .h = 248.f };

        state->video.sdl.sprites.pacman[DIRECTION_RIGHT][2].pos
            = state->video.sdl.sprites.pacman[DIRECTION_LEFT][2].pos
            = state->video.sdl.sprites.pacman[DIRECTION_UP][2].pos
            = state->video.sdl.sprites.pacman[DIRECTION_DOWN][2].pos
            = state->video.sdl.sprites.pacman[DIRECTION_NONE][0].pos
            = state->video.sdl.sprites.pacman[DIRECTION_NONE][1].pos
            = state->video.sdl.sprites.pacman[DIRECTION_NONE][2].pos
            = state->video.sdl.sprites.pacman[DIRECTION_NONE][3].pos
            = state->video.sdl.sprites.pacman_die[0].pos
            = (SDL_FRect){ .x = 488.f, .y = 0.f, .w = 15.f, .h = 15.f };

        state->video.sdl.sprites.pacman[DIRECTION_RIGHT][0].pos
            = (SDL_FRect){ .x = 456.f, .y = 0.f, .w = 15.f, .h = 15.f };
        state->video.sdl.sprites.pacman[DIRECTION_RIGHT][1].pos
            = (SDL_FRect){ .x = 472.f, .y = 0.f, .w = 15.f, .h = 15.f };
        state->video.sdl.sprites.pacman[DIRECTION_RIGHT][3].pos
            = (SDL_FRect){ .x = 472.f, .y = 0.f, .w = 15.f, .h = 15.f };

        state->video.sdl.sprites.pacman[DIRECTION_LEFT][0].pos
            = (SDL_FRect){ .x = 456.f, .y = 16.f, .w = 15.f, .h = 15.f };
        state->video.sdl.sprites.pacman[DIRECTION_LEFT][1].pos
            = (SDL_FRect){ .x = 472.f, .y = 16.f, .w = 15.f, .h = 15.f };
        state->video.sdl.sprites.pacman[DIRECTION_LEFT][3].pos
            = (SDL_FRect){ .x = 472.f, .y = 16.f, .w = 15.f, .h = 15.f };

        state->video.sdl.sprites.pacman[DIRECTION_UP][0].pos
            = (SDL_FRect){ .x = 456.f, .y = 32.f, .w = 15.f, .h = 15.f };
        state->video.sdl.sprites.pacman[DIRECTION_UP][1].pos
            = (SDL_FRect){ .x = 472.f, .y = 32.f, .w = 15.f, .h = 15.f };
        state->video.sdl.sprites.pacman[DIRECTION_UP][3].pos
            = (SDL_FRect){ .x = 472.f, .y = 32.f, .w = 15.f, .h = 15.f };

        state->video.sdl.sprites.pacman[DIRECTION_DOWN][0].pos
            = (SDL_FRect){ .x = 456.f, .y = 48.f, .w = 15.f, .h = 15.f };
        state->video.sdl.sprites.pacman[DIRECTION_DOWN][1].pos
            = (SDL_FRect){ .x = 472.f, .y = 48.f, .w = 15.f, .h = 15.f };
        state->video.sdl.sprites.pacman[DIRECTION_DOWN][3].pos
            = (SDL_FRect){ .x = 472.f, .y = 48.f, .w = 15.f, .h = 15.f };
    }

    for (auto i = 0; i < 4; i++)
    {
        for (auto dir = 0; dir < 4; dir++)
        {
            for (auto j = 0; j < 2; j++)
            {
                state->ghosts.ghost[i].sprite[dir][j] = (SDL_FRect){
                    .x = 456.0f + (float)j * 16.0f + (float)dir * 32.0f,
                    .y = 64.0f + (float)i * 16.0f,
                    .w = 16.0f,
                    .h = 16.0f
                };
            }
        }
    }

    for (auto i = 0; i < sizeof (state->video.sdl.sprites.pacman_die)
                             / sizeof (state->video.sdl.sprites.pacman_die[0]);
         i++)
    {
        if (i
            == sizeof (state->video.sdl.sprites.pacman_die)
                       / sizeof (state->video.sdl.sprites.pacman_die[0])
                   - 1)
        {
            state->video.sdl.sprites.pacman_die[i].pos
                = (SDL_FRect){ .x = 488.f + 16.0f * (float)i,
                      .y = 0.f + 16.0f,
                      .w = 15.f,
                      .h = 15.f };
        }
        else
        {
            state->video.sdl.sprites.pacman_die[i].pos = (SDL_FRect){
                .x = 488.f + 16.0f * i, .y = 0.f, .w = 15.f, .h = 15.f
            };
        }
    }

    {
        nk_init_default (&state->video.nuklear.ctx, nullptr);
        nk_font_atlas_init_default (&state->video.nuklear.atlas);
        nk_font_atlas_begin (&state->video.nuklear.atlas);
        const struct nk_font_config config = nk_font_config (0);
        const auto font = nk_font_atlas_add_default (
            &state->video.nuklear.atlas, 13, &config);
        int img_width = 0;
        int img_height = 0;
        const void *img
            = (void *)nk_font_atlas_bake (&state->video.nuklear.atlas,
                &img_width,
                &img_height,
                NK_FONT_ATLAS_RGBA32);

        state->video.sdl.sprites.font
            = SDL_CreateTexture (state->video.sdl.renderer,
                SDL_PIXELFORMAT_ARGB8888,
                SDL_TEXTUREACCESS_STATIC,
                img_width,
                img_height);
        SDL_UpdateTexture (state->video.sdl.sprites.font,
            NULL,
            img,
            sizeof (uint32_t) * img_width);
        SDL_SetTextureBlendMode (
            state->video.sdl.sprites.font, SDL_BLENDMODE_BLEND);

        nk_font_atlas_end (&state->video.nuklear.atlas,
            nk_handle_ptr (state->video.sdl.sprites.font),
            &state->video.nuklear.tex_null);

        nk_style_set_font (&state->video.nuklear.ctx, &font->handle);

        state->video.nuklear.ctx.clip.copy = nk_sdl_clipboard_copy;
        state->video.nuklear.ctx.clip.paste = nk_sdl_clipboard_paste;
        state->video.nuklear.ctx.clip.userdata = nk_handle_ptr (nullptr);
        nk_buffer_init_default (&state->video.nuklear.cmds);
    }
    init_state (state, init_width, init_height, true);
    return 0;

error:
    return -1;
}

SDL_AppResult SDL_AppIterate (void *appstate)
{
    const auto state = (state_t *)appstate;
    const auto delta = SDL_GetTicks () - state->last_ticks;
    state->delta = delta;
    state->last_ticks = SDL_GetTicks ();
    struct nk_colorf bg;
    bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;
    if (!state->options.pause)
    {
        state->tick++;
    }

    update_ghosts (state);

    if (trigger_triggered (state, &state->pacman.trigger.pacman_animation))
    {
        state->pacman.animation = (state->pacman.animation + 1) % 4;
        trigger_start_after (state,
            &state->pacman.trigger.pacman_animation,
            PACMAN_ANIMATION_TICKS);
    }
    if (trigger_triggered (state, &state->ghosts.animation.trigger))
    {
        state->ghosts.animation.value
            = (state->ghosts.animation.value + 1) % 2;
        trigger_start_after (
            state, &state->ghosts.animation.trigger, GHOST_ANIMATION_TICKS);
    }
    if (trigger_triggered (
            state, &state->pacman.trigger.pacman_move_between_cells))
    {
        state->pacman.position_interp++;
        trigger_start_after (state,
            &state->pacman.trigger.pacman_move_between_cells,
            PACMAN_MOVE_BETWEEN_CELLS_TICKS);
    }

    if (trigger_triggered (state, &state->pacman.trigger.pacman_move))
    {
        auto x = state->pacman.position.x;
        auto y = state->pacman.position.y;
        if (try_move (state, &x, &y, state->pacman.next_direction, false))
        {
            state->pacman.direction = state->pacman.next_direction;
        }
        x = state->pacman.position.x;
        y = state->pacman.position.y;
        const auto direction = state->pacman.direction;
        if (try_move (state, &x, &y, direction, false))
        {
            state->pacman.position.x = x;
            state->pacman.position.y = y;
        }
        else
        {
            state->pacman.direction = DIRECTION_NONE;
        }
        trigger_start_after (
            state, &state->pacman.trigger.pacman_move, PACMAN_MOVE_TICKS);
        state->pacman.position_interp = 0;
    }
    if (trigger_triggered (state, &state->pacman.trigger.pacman_die))
    {
        trigger_stop (&state->pacman.trigger.pacman_animation);
        trigger_stop (&state->pacman.trigger.pacman_move);
        trigger_stop (&state->pacman.trigger.pacman_move_between_cells);
        for (auto i = 0; i < 4; i++)
        {
            trigger_stop (&state->ghosts.ghost[i].trigger.move.trigger);
            trigger_stop (&state->ghosts.ghost[i].trigger.move_between);
        }
        if (!state->pacman.dead)
        {
            state->pacman.dead = true;
            state->pacman.animation = 0;
        }
        else
        {
            constexpr auto PACMAN_DEAD_END_FRAME
                = sizeof (state->video.sdl.sprites.pacman_die)
                    / sizeof (state->video.sdl.sprites.pacman_die[0])
                - 1;
            state->pacman.animation
                = state->pacman.animation == PACMAN_DEAD_END_FRAME
                    ? PACMAN_DEAD_END_FRAME
                    : state->pacman.animation + 1;
        }
        trigger_start_after (state,
            &state->pacman.trigger.pacman_die,
            PACMAN_ANIMATION_TICKS * .6f);
    }

    SDL_SetRenderTarget (
        state->video.sdl.renderer, state->video.sdl.framebuffer_texture);
    {
        SDL_SetRenderDrawColor (state->video.sdl.renderer, 0, 0, 0, 0);
        SDL_RenderClear (state->video.sdl.renderer);
        SDL_RenderTexture (state->video.sdl.renderer,
            state->video.sdl.sprites.atlas,
            &state->video.sdl.sprites.map.pos,
            nullptr);
        if (state->options.draw_map_overlay)
        {
            for (auto y = 0; y < (int)GRID_HEIGHT; y++)
            {
                for (auto x = 0; x < (int)GRID_WIDTH; x++)
                {
                    switch (state->map[coords_to_map_index (y, x)])
                    {
                    case CELL_EMPTY:
                        SDL_SetRenderDrawColor (
                            state->video.sdl.renderer, 100, 100, 100, 125);
                        break;
                    case CELL_WALL:
                        SDL_SetRenderDrawColor (
                            state->video.sdl.renderer, 0, 0, 255, 125);
                        break;
                    case CELL_COIN:
                        SDL_SetRenderDrawColor (
                            state->video.sdl.renderer, 255, 255, 0, 125);
                        break;
                    case CELL_GHOST_WALL:
                        SDL_SetRenderDrawColor (
                            state->video.sdl.renderer, 255, 100, 100, 125);
                        break;
                    }
                    SDL_RenderFillRect (state->video.sdl.renderer,
                        &(SDL_FRect){ .x = x * CELL_WIDTH,
                            .y = y * CELL_HEIGHT,
                            .w = CELL_WIDTH,
                            .h = CELL_HEIGHT });
                }
            }
        }
        {
            int x = state->pacman.position.x;
            int y = state->pacman.position.y;
            double x_offset = 0;
            double y_offset = 0;
            try_move_and_set (
                state, x, y, &x, &y, state->pacman.direction, false);
            {
                x_offset = (double)state->pacman.position.x
                         - lerp (state->pacman.position.x,
                             x,
                             (double)state->pacman.position_interp
                                 / PACMAN_MOVE_TICKS);
                y_offset = (double)state->pacman.position.y
                         - lerp (state->pacman.position.y,
                             y,
                             (double)state->pacman.position_interp
                                 / PACMAN_MOVE_TICKS);
            }

            SDL_RenderTexture (state->video.sdl.renderer,
                state->video.sdl.sprites.atlas,
                state->pacman.dead ? &state->video.sdl.sprites
                                          .pacman_die[state->pacman.animation]
                                          .pos
                                   : &state->video.sdl.sprites
                                          .pacman[state->pacman.direction]
                                                 [state->pacman.animation]
                                          .pos,
                &(SDL_FRect){
                    ((float)state->pacman.position.x + x_offset) * CELL_WIDTH,
                    ((float)state->pacman.position.y + y_offset) * CELL_HEIGHT,
                    CELL_WIDTH,
                    CELL_HEIGHT });
        }
        if (state->options.draw_pacman_cell)
        {
            SDL_SetRenderDrawColor (
                state->video.sdl.renderer, 255, 255, 255, 125);
            SDL_RenderFillRect (state->video.sdl.renderer,
                &(SDL_FRect){ .x = state->pacman.position.x * CELL_WIDTH,
                    .y = state->pacman.position.y * CELL_HEIGHT,
                    .w = CELL_WIDTH,
                    .h = CELL_HEIGHT });
        }

        draw_ghosts (state);
    }
    SDL_SetRenderTarget (state->video.sdl.renderer, nullptr);
    {
        SDL_SetRenderDrawColor (state->video.sdl.renderer, 0, 0, 0, 0);
        SDL_RenderClear (state->video.sdl.renderer);
        SDL_RenderTexture (state->video.sdl.renderer,
            state->video.sdl.framebuffer_texture,
            nullptr,
            &state->video.sdl.target_rect);

        {
            add_nuklear_windows (state);
        }
    }
    SDL_RenderPresent (state->video.sdl.renderer);
    const double rest_of_timeslice
        = MS_PER_FRAME - (SDL_GetTicks () - state->last_ticks);
    SDL_Delay (rest_of_timeslice * (rest_of_timeslice >= 0.0));
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    state_t *state = appstate;
    nk_input_begin (&state->video.nuklear.ctx);
    switch (event->type)
    {
    case SDL_EVENT_QUIT:
        state->running = false;
        return SDL_APP_SUCCESS;
    case SDL_EVENT_KEY_DOWN:
        if (event_key (event, state))
            return SDL_APP_SUCCESS;
        break;
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        [[fallthrough]];
    case SDL_EVENT_WINDOW_RESIZED:
    {
        const int width = event->window.data1;
        const int height = event->window.data2;
        resize_event (state, width, height);
    }
    break;
    default:
        break;
    }
    nk_sdl_handle_event (state, event);
    nk_sdl_handle_grab (state);
    nk_input_end (&state->video.nuklear.ctx);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit (void *appstate)
{
    state_t *state = appstate;
    nk_font_atlas_clear (&state->video.nuklear.atlas);
    nk_free (&state->video.nuklear.ctx);
    SDL_DestroyTexture (state->video.sdl.framebuffer_texture);
    SDL_DestroyTexture (state->video.sdl.sprites.font);
    SDL_DestroyTexture (state->video.sdl.sprites.atlas);
    SDL_DestroyRenderer (state->video.sdl.renderer);
    SDL_DestroyWindow (state->video.sdl.window);
    nk_buffer_free (&state->video.nuklear.cmds);
    free (state);
}

bool event_key (const SDL_Event *event, state_t *state)
{
    switch (event->key.scancode)
    {
    case SDL_SCANCODE_ESCAPE:
        return true;
    case SDL_SCANCODE_LEFT:
        state->pacman.next_direction = DIRECTION_LEFT;
        break;
    case SDL_SCANCODE_RIGHT:
        state->pacman.next_direction = DIRECTION_RIGHT;
        break;
    case SDL_SCANCODE_UP:
        state->pacman.next_direction = DIRECTION_UP;
        break;
    case SDL_SCANCODE_DOWN:
        state->pacman.next_direction = DIRECTION_DOWN;
        break;
    case SDL_SCANCODE_K:
        trigger_start_after (state, &state->pacman.trigger.pacman_die, 1);
        break;
    case SDL_SCANCODE_P:
        state->options.pause = !state->options.pause;
        break;
    case SDL_SCANCODE_F:
    {
        const auto fps = 1000 / state->delta;
        printf ("fps: %u\n", fps);
    }
    break;
    case SDL_SCANCODE_R:
        init_state (
            state, state->video.sdl.width, state->video.sdl.height, false);
        break;
    case SDL_SCANCODE_H:
        state->video.sdl.bordered = !state->video.sdl.bordered;
        SDL_SetWindowBordered (
            state->video.sdl.window, state->video.sdl.bordered);
        SDL_SetWindowResizable (
            state->video.sdl.window, state->video.sdl.bordered);
    default:
        break;
    }
    return false;
}

static void resize_event (state_t *state, const int width, const int height)
{
    state->video.sdl.width = width;
    state->video.sdl.height = height;
    int h;
    int w;
    const auto wanted_w = width < WIDTH ? width : WIDTH;
    const auto wanted_h = height < HEIGHT ? height : HEIGHT;
    if (HEIGHT * GRID_RATIO <= width)
    {
        h = wanted_h;
        w = wanted_h * GRID_RATIO;
    }
    else
    {
        w = wanted_w;
        h = wanted_w * GRID_RATIO_INV;
    }
    state->video.sdl.target_rect.x = (width - w) / 2.0;
    state->video.sdl.target_rect.y = (height - h) / 2.0;
    state->video.sdl.target_rect.w = w;
    state->video.sdl.target_rect.h = h;
}
