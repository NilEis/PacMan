#ifndef UTILS_H
#define UTILS_H
#include "typedefs.h"

bool try_move (
    const state_t *const state, int *x, int *y, const direction_t direction);

bool try_move_and_set (const state_t *const state,
    const int x,
    const int y,
    int *out_x,
    int *out_y,
    const direction_t direction);

void parse_map (state_t *state, const bool load_atlas);

#endif //UTILS_H