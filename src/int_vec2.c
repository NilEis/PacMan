#include "int_vec2.h"

int dist_squared (const int_vec2_t *a, const int_vec2_t *b)
{
    const int_vec2_t c = { .x = a->x - b->x, .y = a->y - b->y };
    return c.x * c.x + c.y * c.y;
}

const int_vec2_t *direction_to_vec (
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

direction_t direction_inv (const direction_t dir)
{
    switch (dir)
    {
    case DIRECTION_RIGHT:
        return DIRECTION_LEFT;
    case DIRECTION_LEFT:
        return DIRECTION_RIGHT;
    case DIRECTION_UP:
        return DIRECTION_DOWN;
    case DIRECTION_DOWN:
        return DIRECTION_UP;
    default:
        break;
    }
    return DIRECTION_NONE;
}