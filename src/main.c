#include "SDL3/SDL_scancode.h"
#include "maps.h"
#define SDL_MAIN_USE_CALLBACKS
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_main.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_timer.h"
#include "SDL3_image/SDL_image.h"
#include "assets.h"
#include "defines.h"
#include "nk_util_functions.h"
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
static double lerp (const double a, const double b, const double t)
{
    return b * (1.0f - t) + a * t;
}

static int dist_squared (const int_vec2_t *a, const int_vec2_t *b)
{
    const int_vec2_t c = { .x = a->x - b->x, .y = a->y - b->y };
    return c.x * c.x + c.y * c.y;
}

bool event_key (const SDL_Event *event, state_t *state);
static void resize_event (state_t *state, const int width, const int height);
static int coords_to_map_index (const int y, const int x)
{
    return y * (int)GRID_WIDTH + x;
}

static bool try_move (
    const state_t *const state, int *x, int *y, const direction_t direction);

static bool test_cell (
    const SDL_Surface *map_surface, const int y, const int x);

static bool try_move_and_set (const state_t *const state,
    int x,
    int y,
    int *out_x,
    int *out_y,
    const direction_t direction);
static void parse_map (state_t *state, bool load_atlas)
{
    SDL_IOStream *file = SDL_IOFromConstMem (
        asset_general_sprites_png, asset_general_sprites_png_size);
    SDL_Surface *full_asset_image = IMG_LoadTyped_IO (file, SDL_TRUE, "PNG");
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
            if (test_cell (map_surface, y, x))
            {
                state->map[coords_to_map_index (y, x)] = CELL_WALL;
            }
        }
    }
    SDL_DestroySurface (full_asset_image);
    SDL_DestroySurface (map_surface);
}

static const int_vec2_t *direction_to_vec (
    const direction_t dir, const bool with_bug);

static direction_t direction_inv (const direction_t dir)
{
    switch (dir)
    {
    case DIRECTION_RIGHT:
        return DIRECTION_LEFT;
        break;
    case DIRECTION_LEFT:
        return DIRECTION_RIGHT;
        break;
    case DIRECTION_UP:
        return DIRECTION_DOWN;
        break;
    case DIRECTION_DOWN:
        return DIRECTION_UP;
        break;
    case DIRECTION_NONE:
        break;
    default:
        break;
    }
    return DIRECTION_NONE;
}

static void init_state (state_t *state,
    const int init_width,
    const int init_height,
    const bool load_atlas)
{
    state->pacman.position.x = 1;
    state->pacman.position.y = 1;
    state->pacman.position_interp = 0;
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
        state->ghosts.ghost[i].pos = (int_vec2_t){ .x = 13 + i, .y = 12 };
        state->ghosts.ghost[i].position_interp = 0;
        state->ghosts.ghost[i].trigger.move.ticks = GHOST_MOVE_TICKS;
    }

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
        trigger_after (state,
            &state->ghosts.ghost[i].trigger.move.trigger,
            state->ghosts.ghost[i].trigger.move.ticks);
        trigger_after (state,
            &state->ghosts.ghost[i].trigger.move_between,
            GHOST_MOVE_BETWEEN_CELLS_TICKS);
    }
}

int SDL_AppInit (void **appstate, int argc, char **argv)
{
    (void)argc;
    (void)argv;
    state_t *state = calloc (1, sizeof (state_t));

    *appstate = state;
    const auto result = SDL_InitSubSystem (SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    if (result < 0)
    {
        goto error;
    }
    SDL_SetHint ("SDL_RENDER_SCALE_QUALITY", "0");
    const SDL_DisplayMode *dm = SDL_GetDesktopDisplayMode (1);
    if (dm == nullptr)
    {
        SDL_Log ("SDL_GetDesktopDisplayMode failed: %s", SDL_GetError ());
    }
    const int init_width = dm ? dm->w : WIDTH;
    const int init_height = dm ? dm->h : HEIGHT;
    state->video.sdl.window = SDL_CreateWindow (NAME,
        init_width,
        init_height,
        SDL_WINDOW_BORDERLESS | SDL_WINDOW_TRANSPARENT | SDL_WINDOW_RESIZABLE);
    if (state->video.sdl.window == NULL)
    {
        goto error;
    }

    SDL_SetWindowFullscreen (state->video.sdl.window, SDL_TRUE);
    state->video.sdl.renderer
        = SDL_CreateRenderer (state->video.sdl.window, nullptr);
    if (state->video.sdl.renderer == nullptr)
    {
        SDL_Log ("SDL_GetDesktopDisplayMode failed: %s", SDL_GetError ());
        goto error;
    }
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
                state->ghosts.ghost[i].sprite[dir][j]
                    = (SDL_FRect){ .x = 456 + j * 16 + dir * 32,
                          .y = 64 + i * 16,
                          .w = 16,
                          .h = 16 };
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
            state->video.sdl.sprites.pacman_die[i].pos = (SDL_FRect){
                .x = 488.f + 16.0f * i, .y = 0.f + 16.0f, .w = 15.f, .h = 15.f
            };
        }
        else
        {
            state->video.sdl.sprites.pacman_die[i].pos = (SDL_FRect){
                .x = 488.f + 16.0f * i, .y = 0.f, .w = 15.f, .h = 15.f
            };
        }
    }

    {
        nk_init_default (&state->video.nuklear.ctx, NULL);
        nk_font_atlas_init_default (&state->video.nuklear.atlas);
        nk_font_atlas_begin (&state->video.nuklear.atlas);
        const struct nk_font_config config = nk_font_config (0);
        auto font = nk_font_atlas_add_default (
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

int SDL_AppIterate (void *appstate)
{
    const auto state = (state_t *)appstate;
    const auto delta = SDL_GetTicks () - state->last_ticks;
    struct nk_colorf bg;
    bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;
    state->delta = delta;
    state->last_ticks = SDL_GetTicks ();
    state->tick++;

    {
        const int_vec2_t *pacman_dir
            = direction_to_vec (state->pacman.direction, true);
        for (auto i = 0; i < 4; i++)
        {
            if (dist_squared (&state->ghosts.ghost[i].pos, &state->pacman.position) <= 2)
            {
                trigger_start_after (state, &state->pacman.trigger.pacman_die,1);
            }
            switch (i)
            {
            case GHOST_BLINKY:
            {
                state->ghosts.ghost[i].target.x = state->pacman.position.x;
                state->ghosts.ghost[i].target.y = state->pacman.position.y;
            }
            break;
            case GHOST_PINKY:
            {
                state->ghosts.ghost[i].target.x
                    = state->pacman.position.x + 4 * pacman_dir->x;
                state->ghosts.ghost[i].target.y
                    = state->pacman.position.y + 4 * pacman_dir->y;
            }
            break;
            case GHOST_INKY:
            {
                const int_vec2_t pac_offset
                    = { .x = state->pacman.position.x + pacman_dir->x * 2,
                          .y = state->pacman.position.y + pacman_dir->y * 2 };
                const int_vec2_t blinky_to_pac_offset = {
                    .x
                    = pac_offset.x - state->ghosts.ghost[GHOST_BLINKY].pos.x,
                    .y = pac_offset.y - state->ghosts.ghost[GHOST_BLINKY].pos.y
                };
                state->ghosts.ghost[i].target.x
                    = state->ghosts.ghost[GHOST_BLINKY].pos.x
                    + blinky_to_pac_offset.x * 2;
                state->ghosts.ghost[i].target.y
                    = state->ghosts.ghost[GHOST_BLINKY].pos.y
                    + blinky_to_pac_offset.y * 2;
            }
            break;
            case GHOST_CLYDE:
            {
                if (dist_squared (
                        &state->ghosts.ghost[i].pos, &state->pacman.position)
                    >= 8 * 8)
                {
                    state->ghosts.ghost[i].target.x
                        = state->ghosts.ghost[GHOST_BLINKY].target.x;
                    state->ghosts.ghost[i].target.y
                        = state->ghosts.ghost[GHOST_BLINKY].target.y;
                }
                else
                {
                    state->ghosts.ghost[i].target.x
                        = state->ghosts.ghost[i].corner.x;
                    state->ghosts.ghost[i].target.y
                        = state->ghosts.ghost[i].corner.y;
                }
            }
            break;
            default:
                break;
            }
            if (trigger_triggered (
                    state, &state->ghosts.ghost[i].trigger.move_between))
            {
                state->ghosts.ghost[i].position_interp++;
                trigger_after (state,
                    &state->ghosts.ghost[i].trigger.move_between,
                    GHOST_MOVE_BETWEEN_CELLS_TICKS);
            }
            if (trigger_triggered (
                    state, &state->ghosts.ghost[i].trigger.move.trigger))
            {
                state->ghosts.ghost[i].position_interp = 0;
                constexpr direction_t direction_prio[4] = { DIRECTION_UP,
                    DIRECTION_LEFT,
                    DIRECTION_DOWN,
                    DIRECTION_RIGHT };
                int min_dist = INT_MAX;
                direction_t min_dir = DIRECTION_NONE;
                int_vec2_t min_pos = { 0 };
                for (auto d = 0; d < 4; d++)
                {
                    if (direction_inv (state->ghosts.ghost[i].dir)
                        == direction_prio[d])
                    {
                        continue;
                    }
                    int_vec2_t n_pos = { .x = state->ghosts.ghost[i].pos.x,
                        .y = state->ghosts.ghost[i].pos.y };
                    if (!try_move (
                            state, &n_pos.x, &n_pos.y, direction_prio[d]))
                    {
                        continue;
                    }
                    const int new_dist = dist_squared (
                        &n_pos, &state->ghosts.ghost[i].target);
                    if (new_dist < min_dist)
                    {
                        min_dist = new_dist;
                        min_dir = direction_prio[d];
                        min_pos = n_pos;
                    }
                }
                state->ghosts.ghost[i].dir = min_dir;
                state->ghosts.ghost[i].pos = min_pos;
                trigger_start_after (state,
                    &state->ghosts.ghost[i].trigger.move.trigger,
                    state->ghosts.ghost[i].trigger.move.ticks);
            }
        }
    }

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
        if (try_move (state, &x, &y, state->pacman.next_direction))
        {
            state->pacman.direction = state->pacman.next_direction;
        }
        x = state->pacman.position.x;
        y = state->pacman.position.y;
        const auto direction = state->pacman.direction;
        if (try_move (state, &x, &y, direction))
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
        trigger_start_after (
            state, &state->pacman.trigger.pacman_die, PACMAN_ANIMATION_TICKS);
    }

    SDL_SetRenderTarget (
        state->video.sdl.renderer, state->video.sdl.framebuffer_texture);
    {
        SDL_SetRenderDrawColor (state->video.sdl.renderer, 0, 0, 0, 0);
        SDL_RenderClear (state->video.sdl.renderer);
        SDL_RenderTexture (state->video.sdl.renderer,
            state->video.sdl.sprites.atlas,
            &state->video.sdl.sprites.map.pos,
            NULL);
        {
            int x = state->pacman.position.x;
            int y = state->pacman.position.y;
            double x_offset = 0;
            double y_offset = 0;
            try_move_and_set (state, x, y, &x, &y, state->pacman.direction);
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
                state->pacman.dead
                    ? (&state->video.sdl.sprites
                              .pacman_die[state->pacman.animation]
                              .pos)
                    : (&state->video.sdl.sprites
                              .pacman[state->pacman.direction]
                                     [state->pacman.animation]
                              .pos),
                &(SDL_FRect){
                    ((float)state->pacman.position.x + x_offset) * CELL_WIDTH,
                    ((float)state->pacman.position.y + y_offset) * CELL_HEIGHT,
                    CELL_WIDTH,
                    CELL_HEIGHT });
        }
        for (auto i = 0; i < 4; i++)
        {
            int x = state->ghosts.ghost[i].pos.x;
            int y = state->ghosts.ghost[i].pos.y;
            double x_offset = 0;
            double y_offset = 0;
            try_move_and_set (state, x, y, &x, &y, state->ghosts.ghost[i].dir);
            {
                x_offset = (double)state->ghosts.ghost[i].pos.x
                         - lerp (state->ghosts.ghost[i].pos.x,
                             x,
                             (double)state->ghosts.ghost[i].position_interp
                                 / state->ghosts.ghost[i].trigger.move.ticks);
                y_offset = (double)state->ghosts.ghost[i].pos.y
                         - lerp (state->ghosts.ghost[i].pos.y,
                             y,
                             (double)state->ghosts.ghost[i].position_interp
                                 / state->ghosts.ghost[i].trigger.move.ticks);
            }

            SDL_RenderTexture (state->video.sdl.renderer,
                state->video.sdl.sprites.atlas,
                &state->ghosts.ghost[i].sprite[state->ghosts.ghost[i].dir]
                                              [state->ghosts.animation.value],
                &(SDL_FRect){ ((float)state->ghosts.ghost[i].pos.x + x_offset)
                                  * CELL_WIDTH,
                    ((float)state->ghosts.ghost[i].pos.y + y_offset)
                        * CELL_HEIGHT,
                    CELL_WIDTH,
                    CELL_HEIGHT });
            if (false)
            {
                SDL_SetRenderDrawColor (state->video.sdl.renderer,
                    state->ghosts.ghost[i].color.r,
                    state->ghosts.ghost[i].color.g,
                    state->ghosts.ghost[i].color.b,
                    255);
                SDL_RenderLines (state->video.sdl.renderer,
                    (SDL_FPoint[]){
                        (SDL_FPoint){ state->ghosts.ghost[i].pos.x * CELL_WIDTH
                                          - CELL_WIDTH / 2,
                                     state->ghosts.ghost[i].pos.y * CELL_HEIGHT
                                + CELL_HEIGHT / 2 },
                        (SDL_FPoint){ state->ghosts.ghost[i].pos.x * CELL_WIDTH
                                          - CELL_WIDTH / 2,
                                     state->ghosts.ghost[i].target.y * CELL_HEIGHT
                                + CELL_HEIGHT / 2 },
                        (SDL_FPoint){
                                     state->ghosts.ghost[i].target.x * CELL_WIDTH
                                - CELL_WIDTH / 2,
                                     state->ghosts.ghost[i].target.y * CELL_HEIGHT
                                + CELL_HEIGHT / 2 }
                },
                    3);
                SDL_RenderFillRect (state->video.sdl.renderer,
                    &(SDL_FRect){
                        .x = state->ghosts.ghost[i].target.x * CELL_WIDTH,
                        .y = state->ghosts.ghost[i].target.y * CELL_HEIGHT,
                        .w = CELL_WIDTH,
                        .h = CELL_HEIGHT });
            }
        }
    }
    SDL_SetRenderTarget (state->video.sdl.renderer, NULL);
    {
        SDL_SetRenderDrawColor (state->video.sdl.renderer, 0, 0, 0, 0);
        SDL_RenderClear (state->video.sdl.renderer);
        SDL_RenderTexture (state->video.sdl.renderer,
            state->video.sdl.framebuffer_texture,
            NULL,
            &state->video.sdl.target_rect);

        {
            if (nk_begin (&state->video.nuklear.ctx,
                    "debug",
                    (struct nk_rect){ 10, 10, 400, 300 },
                    NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE
                        | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE
                        | NK_WINDOW_SCROLL_AUTO_HIDE))
            {
            }
            nk_end (&state->video.nuklear.ctx);
            nk_render (state, NK_ANTI_ALIASING_ON);
        }
    }
    SDL_RenderPresent (state->video.sdl.renderer);
    while (SDL_GetTicks () - state->last_ticks < TICK_SPEED)
    {
    }
    return SDL_APP_CONTINUE;
}

int SDL_AppEvent (void *appstate, const SDL_Event *event)
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
    int h = 0;
    int w = 0;
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

static bool test_cell (
    const SDL_Surface *map_surface, const int y, const int x)
{
    for (auto yp = 0; yp < TILE_HEIGHT; yp++)
    {
        for (auto xp = 0; xp < TILE_WIDTH; xp++)
        {
            uint8_t r = 1;
            uint8_t g = 2;
            uint8_t b = 3;
            const auto pixel
                = *(uint32_t *)((uint8_t *)map_surface->pixels
                                + (y * TILE_HEIGHT + yp) * map_surface->pitch
                                + (x * TILE_WIDTH + xp)
                                      * map_surface->format->bytes_per_pixel);
            SDL_GetRGB (pixel, map_surface->format, &r, &g, &b);
            const auto i = (int)r << 16 | (int)g << 8 | b;
            if (b != 0)
            {
                return true;
            }
        }
    }
    return false;
}

static bool try_move (
    const state_t *const state, int *x, int *y, const direction_t direction)
{
    int cx = 0;
    int cy = 0;
    bool res = false;
    if (try_move_and_set (state, *x, *y, &cx, &cy, direction))
    {
        *x = cx;
        *y = cy;
        res = true;
    }
    return res;
}

static bool try_move_and_set (const state_t *const state,
    int x,
    int y,
    int *out_x,
    int *out_y,
    const direction_t direction)
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
        break;
    }
    cx = (int)((cx + (uint32_t)GRID_WIDTH) % (uint32_t)GRID_WIDTH);
    cy = (int)((cy + (uint32_t)GRID_HEIGHT) % (uint32_t)GRID_HEIGHT);
    *out_x = cx;
    *out_y = cy;
    if (state->map[coords_to_map_index (cy, cx)] != CELL_WALL
        && state->map[coords_to_map_index (cy, cx)] != CELL_GHOST_WALL)
    {
        res = true;
    }
    return res;
}

static const int_vec2_t *direction_to_vec (
    const direction_t dir, const bool with_bug)
{
    static constexpr auto right = (int_vec2_t){ .x = 1, .y = 0 };
    static constexpr auto left = (int_vec2_t){ .x = -1, .y = 0 };
    static constexpr auto up = (int_vec2_t){ .x = 0, .y = -1 };
    static constexpr auto up_bug = (int_vec2_t){ .x = -1, .y = -1 };
    static constexpr auto down = (int_vec2_t){ .x = 0, .y = 1 };
    static constexpr auto none = (int_vec2_t){ .x = 0, .y = 0 };
    switch (dir)
    {
    case DIRECTION_RIGHT:
        return &right;
        break;
    case DIRECTION_LEFT:
        return &left;
        break;
    case DIRECTION_UP:
        return with_bug ? &up_bug : &up;
        break;
    case DIRECTION_DOWN:
        return &down;
        break;
    case DIRECTION_NONE:
        return &none;
        break;
    default:
        return &none;
        break;
    }
}
