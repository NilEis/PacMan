#ifndef INT_VEC2_H
#define INT_VEC2_H

#include "typedefs.h"

typedef struct
{
    int x;
    int y;
} int_vec2_t;

int dist_squared (const int_vec2_t *a, const int_vec2_t *b);
const int_vec2_t *direction_to_vec (direction_t dir, bool with_bug);
direction_t direction_inv (const direction_t dir);

#endif //INT_VEC2_H
