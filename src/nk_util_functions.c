#define NK_IMPLEMENTATION
#define NK_PRIVATE
#include "nuklear_default.h"
#include "nuklear.h"

static int SDL_RenderGeometryRaw8BitColor (SDL_Renderer *renderer,
    SDL_Texture *texture,
    const float *xy,
    int xy_stride,
    const SDL_Color *color,
    int color_stride,
    const float *uv,
    int uv_stride,
    int num_vertices,
    const void *indices,
    int num_indices,
    int size_indices)
{
    int i, retval;
    const Uint8 *color2 = (const Uint8 *)color;
    SDL_FColor *color3;

    if (num_vertices <= 0)
    {
        return SDL_InvalidParamError ("num_vertices");
    }
    if (!color)
    {
        return SDL_InvalidParamError ("color");
    }

    color3 = calloc (num_vertices, sizeof (SDL_FColor));
    if (!color3)
    {
        return -1;
    }

    for (i = 0; i < num_vertices; ++i)
    {
        color3[i].r = color->r / 255.0f;
        color3[i].g = color->g / 255.0f;
        color3[i].b = color->b / 255.0f;
        color3[i].a = color->a / 255.0f;
        color2 += color_stride;
        color = (const SDL_Color *)color2;
    }

    retval = SDL_RenderGeometryRaw (renderer,
        texture,
        xy,
        xy_stride,
        color3,
        sizeof (*color3),
        uv,
        uv_stride,
        num_vertices,
        indices,
        num_indices,
        size_indices);
    free (color3);
    return retval;
}

void nk_render (state_t *state, const enum nk_anti_aliasing AA)
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

        SDL_RenderGeometryRaw8BitColor (state->video.sdl.renderer,
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

void nk_sdl_clipboard_paste (const nk_handle usr, struct nk_text_edit *edit)
{
    const char *text = SDL_GetClipboardText ();
    if (text)
        nk_textedit_paste (edit, text, nk_strlen (text));
    (void)usr;
}

void nk_sdl_clipboard_copy (
    const nk_handle usr, const char *text, const int len)
{
    char *str;
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

void nk_sdl_handle_grab (state_t *state)
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

int nk_sdl_handle_event (state_t *state, const SDL_Event *evt)
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
        const nk_bool down = evt->type == SDL_EVENT_MOUSE_BUTTON_DOWN;
        const int x = evt->button.x, y = evt->button.y;
        switch (evt->button.button)
        {
        case SDL_BUTTON_LEFT:
            if (evt->button.clicks > 1)
            {
                nk_input_button (ctx, NK_BUTTON_DOUBLE, x, y, down);
            }
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