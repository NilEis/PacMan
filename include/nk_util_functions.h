#ifndef NK_UTIL_FUNCTIONS_H
#define NK_UTIL_FUNCTIONS_H

#include "nuklear_default.h"
#include "typedefs.h"

void nk_render (state_t *state, enum nk_anti_aliasing AA);

void nk_sdl_clipboard_paste (nk_handle usr, struct nk_text_edit *edit);

void nk_sdl_clipboard_copy (
    nk_handle usr, const char *text, int len);

void nk_sdl_handle_grab (state_t *state);

int nk_sdl_handle_event (state_t *state, const SDL_Event *evt);

#endif // NK_UTIL_FUNCTIONS_H
