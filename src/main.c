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
#define NK_IMPLEMENTATION
#define NK_PRIVATE
#define NK_ASSERT(x)                                                          \
    do                                                                        \
    {                                                                         \
        if (!(x))                                                             \
        {                                                                     \
            *(int *)0 = 0;                                                    \
        }                                                                     \
    }                                                                         \
    while (0)
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_STANDARD_BOOL
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#include "nuklear.h"

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
static void resize_event (state_t *state, const int width, const int height);
static int coords_to_map_index (const int y, const int x)
{
    return y * (int)GRID_WIDTH + x;
}

static void nk_render (state_t *state, enum nk_anti_aliasing AA);
static void nk_sdl_clipboard_paste (
    const nk_handle usr, struct nk_text_edit *edit);
static void nk_sdl_clipboard_copy (
    const nk_handle usr, const char *text, const int len);

static bool test_cell (
    const SDL_Surface *map_surface, const int y, const int x);

static void nk_sdl_handle_grab (state_t *state);

static int nk_sdl_handle_event (state_t *state, const SDL_Event *evt);

int SDL_AppInit (void **appstate, int argc, char **argv)
{
    (void)argc;
    (void)argv;
    state_t *state = calloc (1, sizeof (state_t));
    state->pacman.position.x = 1;
    state->pacman.position.y = 1;
    state->pacman.direction = DIRECTION_LEFT;
    state->pacman.next_direction = DIRECTION_NONE;
    state->pacman.animation = 0;
    memcpy (state->map, map1, sizeof (state->map));

    trigger_stop (&state->trigger.pacman_animation);
    trigger_stop (&state->trigger.pacman_move);

    *appstate = state;
    const auto result = SDL_InitSubSystem (SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    if (result < 0)
    {
        goto error;
    }
    SDL_SetHint ("SDL_RENDER_SCALE_QUALITY", "0");
    state->video.sdl.window = SDL_CreateWindow (NAME,
        WIDTH,
        HEIGHT,
        SDL_WINDOW_BORDERLESS | SDL_WINDOW_TRANSPARENT | SDL_WINDOW_RESIZABLE);
    if (state->video.sdl.window == NULL)
    {
        goto error;
    }
    resize_event (state, WIDTH, HEIGHT);
    state->video.sdl.bordered = false;
    state->video.sdl.width = WIDTH;
    state->video.sdl.width = HEIGHT;
    SDL_SetWindowFullscreen (state->video.sdl.window, SDL_TRUE);
    state->video.sdl.renderer
        = SDL_CreateRenderer (state->video.sdl.window, NULL);
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

    {
        SDL_IOStream *file = SDL_IOFromConstMem (
            asset_general_sprites_png, asset_general_sprites_png_size);
        SDL_Surface *full_asset_image
            = IMG_LoadTyped_IO (file, SDL_TRUE, "PNG");
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
    struct nk_colorf bg;
    bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;
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
        x = (int)((x + (uint32_t)GRID_WIDTH) % (uint32_t)GRID_WIDTH);
        y = (int)((y + (uint32_t)GRID_HEIGHT) % (uint32_t)GRID_HEIGHT);
        if (state->map[coords_to_map_index (y, x)] != CELL_WALL
            && state->map[coords_to_map_index (y, x)] != CELL_GHOST_WALL)
        {
            state->pacman.position.x = x;
            state->pacman.position.y = y;
        }
        trigger_start_after (
            state, &state->trigger.pacman_move, PACMAN_MOVE_TICKS);
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
        SDL_RenderTexture (state->video.sdl.renderer,
            state->video.sdl.sprites.atlas,
            &state->video.sdl.sprites
                 .pacman[state->pacman.direction][state->pacman.animation]
                 .pos,
            &(SDL_FRect){ (float)state->pacman.position.x * CELL_WIDTH,
                (float)state->pacman.position.y * CELL_HEIGHT,
                CELL_WIDTH,
                CELL_HEIGHT });
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
                    (struct nk_rect){ 10, 10, 100, 100 },
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
            if (x == 1 && y == 1)
            {
                printf ("(%u, %u, %u) ", r, g, b);
            }
            if (b != 0)
            {
                return true;
            }
        }
        if (x == 1 && y == 1)
        {
            printf ("\n");
        }
    }
    return false;
}

static void nk_render (state_t *state, const enum nk_anti_aliasing AA)
{
    SDL_Rect saved_clip;
    const SDL_bool clipping_enabled
        = SDL_RenderClipEnabled (state->video.sdl.renderer);

    const struct nk_draw_command *cmd;
    const nk_draw_index *offset = NULL;
    struct nk_buffer vbuf, ebuf;
    /* fill converting configuration */
    struct nk_convert_config config;
    static const struct nk_draw_vertex_layout_element vertex_layout[] = {
        { NK_VERTEX_POSITION,
         NK_FORMAT_FLOAT, NK_OFFSETOF (nk_sdl_vertex_t, position) },
        { NK_VERTEX_TEXCOORD,
         NK_FORMAT_FLOAT, NK_OFFSETOF (nk_sdl_vertex_t, uv) },
        { NK_VERTEX_COLOR,
         NK_FORMAT_R8G8B8A8, NK_OFFSETOF (nk_sdl_vertex_t, col) },
        { NK_VERTEX_LAYOUT_END }
    };

    NK_MEMSET (&config, 0, sizeof (config));
    config.vertex_layout = vertex_layout;
    config.vertex_size = sizeof (nk_sdl_vertex_t);
    config.vertex_alignment = NK_ALIGNOF (nk_sdl_vertex_t);
    config.tex_null = state->video.nuklear.tex_null;
    config.circle_segment_count = 22;
    config.curve_segment_count = 22;
    config.arc_segment_count = 22;
    config.global_alpha = 1.0f;
    config.shape_AA = AA;
    config.line_AA = AA;

    /* convert shapes into vertexes */
    nk_buffer_init_default (&vbuf);
    nk_buffer_init_default (&ebuf);
    nk_convert (&state->video.nuklear.ctx,
        &state->video.nuklear.cmds,
        &vbuf,
        &ebuf,
        &config);

    /* iterate over and execute each draw command */
    offset = (const nk_draw_index *)nk_buffer_memory_const (&ebuf);

    SDL_GetRenderClipRect (state->video.sdl.renderer, &saved_clip);
    nk_draw_foreach (
        cmd, &state->video.nuklear.ctx, &state->video.nuklear.cmds)
    {
        constexpr size_t vc = offsetof (nk_sdl_vertex_t, col);
        constexpr size_t vt = offsetof (nk_sdl_vertex_t, uv);
        constexpr size_t vp = offsetof (nk_sdl_vertex_t, position);
        constexpr int vs = sizeof (nk_sdl_vertex_t);
        if (!cmd->elem_count)
        {
            continue;
        }
        SDL_Rect r;
        r.x = cmd->clip_rect.x;
        r.y = cmd->clip_rect.y;
        r.w = cmd->clip_rect.w;
        r.h = cmd->clip_rect.h;
        SDL_SetRenderClipRect (state->video.sdl.renderer, &r);

        const void *vertices = nk_buffer_memory_const (&vbuf);

        SDL_RenderGeometryRaw (state->video.sdl.renderer,
            cmd->texture.ptr,
            (const float *)((const nk_byte *)vertices + vp),
            vs,
            (const SDL_Color *)((const nk_byte *)vertices + vc),
            vs,
            (const float *)((const nk_byte *)vertices + vt),
            vs,
            (vbuf.needed / vs),
            (void *)offset,
            cmd->elem_count,
            2);

        offset += cmd->elem_count;
    }
    SDL_SetRenderClipRect (state->video.sdl.renderer, &saved_clip);
    if (!clipping_enabled)
    {
        SDL_SetRenderClipRect (state->video.sdl.renderer, NULL);
    }

    nk_clear (&state->video.nuklear.ctx);
    nk_buffer_clear (&state->video.nuklear.cmds);
    nk_buffer_free (&vbuf);
    nk_buffer_free (&ebuf);
}

static void nk_sdl_clipboard_paste (
    const nk_handle usr, struct nk_text_edit *edit)
{
    const char *text = SDL_GetClipboardText ();
    if (text)
        nk_textedit_paste (edit, text, nk_strlen (text));
    (void)usr;
}

static void nk_sdl_clipboard_copy (
    const nk_handle usr, const char *text, const int len)
{
    char *str = nullptr;
    (void)usr;
    if (!len)
        return;
    str = (char *)malloc ((size_t)len + 1);
    if (!str)
        return;
    memcpy (str, text, (size_t)len);
    str[len] = '\0';
    SDL_SetClipboardText (str);
    free (str);
}

static void nk_sdl_handle_grab (state_t *state)
{
    struct nk_context *ctx = &state->video.nuklear.ctx;
    if (ctx->input.mouse.grab)
    {
        SDL_SetRelativeMouseMode (SDL_TRUE);
    }
    else if (ctx->input.mouse.ungrab)
    {
        /* better support for older SDL by setting mode first; causes an extra
         * mouse motion event */
        SDL_SetRelativeMouseMode (SDL_FALSE);
        SDL_WarpMouseInWindow (state->video.sdl.window,
            (int)ctx->input.mouse.prev.x,
            (int)ctx->input.mouse.prev.y);
    }
    else if (ctx->input.mouse.grabbed)
    {
        ctx->input.mouse.pos.x = ctx->input.mouse.prev.x;
        ctx->input.mouse.pos.y = ctx->input.mouse.prev.y;
    }
}

static int nk_sdl_handle_event (state_t *state, const SDL_Event *evt)
{
    struct nk_context *ctx = &state->video.nuklear.ctx;

    switch (evt->type)
    {
    case SDL_EVENT_KEY_UP: /* KEYUP & KEYDOWN share same routine */
        [[fallthrough]];
    case SDL_EVENT_KEY_DOWN:
    {
        const int down = evt->type == SDL_EVENT_KEY_DOWN;
        const Uint8 *key_state = SDL_GetKeyboardState (nullptr);
        switch (evt->key.key)
        {
        case SDLK_RSHIFT: /* RSHIFT & LSHIFT share same routine */
            [[fallthrough]];
        case SDLK_LSHIFT:
            nk_input_key (ctx, NK_KEY_SHIFT, down);
            break;
        case SDLK_DELETE:
            nk_input_key (ctx, NK_KEY_DEL, down);
            break;
        case SDLK_RETURN:
            nk_input_key (ctx, NK_KEY_ENTER, down);
            break;
        case SDLK_TAB:
            nk_input_key (ctx, NK_KEY_TAB, down);
            break;
        case SDLK_BACKSPACE:
            nk_input_key (ctx, NK_KEY_BACKSPACE, down);
            break;
        case SDLK_HOME:
            nk_input_key (ctx, NK_KEY_TEXT_START, down);
            nk_input_key (ctx, NK_KEY_SCROLL_START, down);
            break;
        case SDLK_END:
            nk_input_key (ctx, NK_KEY_TEXT_END, down);
            nk_input_key (ctx, NK_KEY_SCROLL_END, down);
            break;
        case SDLK_PAGEDOWN:
            nk_input_key (ctx, NK_KEY_SCROLL_DOWN, down);
            break;
        case SDLK_PAGEUP:
            nk_input_key (ctx, NK_KEY_SCROLL_UP, down);
            break;
        case SDLK_z:
            nk_input_key (
                ctx, NK_KEY_TEXT_UNDO, down && key_state[SDL_SCANCODE_LCTRL]);
            break;
        case SDLK_r:
            nk_input_key (
                ctx, NK_KEY_TEXT_REDO, down && key_state[SDL_SCANCODE_LCTRL]);
            break;
        case SDLK_c:
            nk_input_key (
                ctx, NK_KEY_COPY, down && key_state[SDL_SCANCODE_LCTRL]);
            break;
        case SDLK_v:
            nk_input_key (
                ctx, NK_KEY_PASTE, down && key_state[SDL_SCANCODE_LCTRL]);
            break;
        case SDLK_x:
            nk_input_key (
                ctx, NK_KEY_CUT, down && key_state[SDL_SCANCODE_LCTRL]);
            break;
        case SDLK_b:
            nk_input_key (ctx,
                NK_KEY_TEXT_LINE_START,
                down && key_state[SDL_SCANCODE_LCTRL]);
            break;
        case SDLK_e:
            nk_input_key (ctx,
                NK_KEY_TEXT_LINE_END,
                down && key_state[SDL_SCANCODE_LCTRL]);
            break;
        case SDLK_UP:
            nk_input_key (ctx, NK_KEY_UP, down);
            break;
        case SDLK_DOWN:
            nk_input_key (ctx, NK_KEY_DOWN, down);
            break;
        case SDLK_LEFT:
            if (key_state[SDL_SCANCODE_LCTRL])
                nk_input_key (ctx, NK_KEY_TEXT_WORD_LEFT, down);
            else
                nk_input_key (ctx, NK_KEY_LEFT, down);
            break;
        case SDLK_RIGHT:
            if (key_state[SDL_SCANCODE_LCTRL])
                nk_input_key (ctx, NK_KEY_TEXT_WORD_RIGHT, down);
            else
                nk_input_key (ctx, NK_KEY_RIGHT, down);
            break;
        default:
            break;
        }
        return 1;
    }

    case SDL_EVENT_MOUSE_BUTTON_UP: /* MOUSEBUTTONUP & MOUSEBUTTONDOWN share
                                       same routine */
        [[fallthrough]];
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    {
        const int down = evt->type == SDL_EVENT_MOUSE_BUTTON_DOWN;
        const int x = evt->button.x, y = evt->button.y;
        switch (evt->button.button)
        {
        case SDL_BUTTON_LEFT:
            if (evt->button.clicks > 1)
                nk_input_button (ctx, NK_BUTTON_DOUBLE, x, y, down);
            nk_input_button (ctx, NK_BUTTON_LEFT, x, y, down);
            break;
        case SDL_BUTTON_MIDDLE:
            nk_input_button (ctx, NK_BUTTON_MIDDLE, x, y, down);
            break;
        case SDL_BUTTON_RIGHT:
            nk_input_button (ctx, NK_BUTTON_RIGHT, x, y, down);
            break;
        default:
            break;
        }
        return 1;
    }

    case SDL_EVENT_MOUSE_MOTION:
    {
        if (ctx->input.mouse.grabbed)
        {
            int x = (int)ctx->input.mouse.prev.x,
                y = (int)ctx->input.mouse.prev.y;
            nk_input_motion (ctx, x + evt->motion.xrel, y + evt->motion.yrel);
        }
        else
            nk_input_motion (ctx, evt->motion.x, evt->motion.y);
        return 1;
    }

    case SDL_EVENT_TEXT_INPUT:
    {
        nk_glyph glyph;
        memcpy (glyph, evt->text.text, NK_UTF_SIZE);
        nk_input_glyph (ctx, glyph);
        return 1;
    }

    case SDL_EVENT_MOUSE_WHEEL:
    {
        nk_input_scroll (ctx, nk_vec2 (evt->wheel.x, evt->wheel.y));
        return 1;
    }
    default:
        break;
    }
    return 0;
}
