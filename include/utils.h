#ifndef UTILS_H
#define UTILS_H
#include "typedefs.h"

bool try_move (const state_t *const state,
    int *x,
    int *y,
    const direction_t direction,
    bool is_ghost);

bool try_move_and_set (const state_t *const state,
    const int x,
    const int y,
    int *out_x,
    int *out_y,
    const direction_t direction,
    bool is_ghost);

void parse_map (state_t *state, const bool load_atlas);

#ifdef UTILS_PRIVATE_FUNCS
#include <stdio.h>
#include <stdlib.h>
static int coords_to_map_index (const int y, const int x)
{
    return y * (int)GRID_WIDTH + x;
}

static double lerp (const double a, const double b, const double t)
{
    const auto t_d = t > 1.0f ? 1.0f : t < 0.0f ? 0.0f : t;
    return b * (1.0f - t_d) + a * t_d;
}
static char *file_to_str (FILE *f, size_t *len)
{
    const auto pos = ftell (f);
    fflush (stdout);
    fseek (f, 0, SEEK_END);
    const auto end = ftell (f);
    rewind (f);
    char *stdout_buf = calloc (end + 1, sizeof (char));
    fread (stdout_buf, sizeof (char), end, f);
    fseek (f, pos, SEEK_SET);
    if (len != NULL)
    {
        *len = end;
    }
    return stdout_buf;
}
#endif
#undef UTILS_H
#endif // UTILS_H
