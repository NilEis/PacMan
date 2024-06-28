#ifndef NK_UTIL_FUNCTIONS_H
#define NK_UTIL_FUNCTIONS_H

// #define NK_ASSERT(x)                                                          \
//     do                                                                        \
//     {                                                                         \
//         if (!(x))                                                             \
//         {                                                                     \
//             *(int *)0 = 0;                                                    \
//         }                                                                     \
//     }                                                                         \
//     while (0)
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_STANDARD_BOOL
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#include "nuklear.h"

#include <SDL3/SDL_clipboard.h>

void nk_render (state_t *state, const enum nk_anti_aliasing AA);

void nk_sdl_clipboard_paste (const nk_handle usr, struct nk_text_edit *edit);

void nk_sdl_clipboard_copy (
    const nk_handle usr, const char *text, const int len);

void nk_sdl_handle_grab (state_t *state);

int nk_sdl_handle_event (state_t *state, const SDL_Event *evt);

#endif // NK_UTIL_FUNCTIONS_H
