#include "ghosts.h"
#include "trigger.h"
#include "utils.h"
#include <limits.h>

static double lerp (const double a, const double b, const double t)
{
    return b * (1.0f - t) + a * t;
}

void update_ghosts (state_t *const state)
{
    const int_vec2_t *pacman_dir
        = direction_to_vec (state->pacman.direction, true);
    for (auto i = 0; i < 4; i++)
    {
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
                .x = pac_offset.x - state->ghosts.ghost[GHOST_BLINKY].pos.x,
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
            constexpr direction_t direction_prio[4] = {
                DIRECTION_UP, DIRECTION_LEFT, DIRECTION_DOWN, DIRECTION_RIGHT
            };
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
                if (!try_move (state, &n_pos.x, &n_pos.y, direction_prio[d], true))
                {
                    continue;
                }
                const int new_dist
                    = dist_squared (&n_pos, &state->ghosts.ghost[i].target);
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
        if (!state->pacman.dead
            && (state->ghosts.ghost[i].pos.x == state->pacman.position.x
                && state->ghosts.ghost[i].pos.y == state->pacman.position.y))
        {
            trigger_start_after (state, &state->pacman.trigger.pacman_die, 0);
        }
    }
}

void draw_ghosts (state_t *const state)
{
    for (auto i = 0; i < 4; i++)
    {
        int x = state->ghosts.ghost[i].pos.x;
        int y = state->ghosts.ghost[i].pos.y;
        double x_offset = 0;
        double y_offset = 0;
        try_move_and_set (state, x, y, &x, &y, state->ghosts.ghost[i].dir, true);
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
            &(SDL_FRect){
                ((float)state->ghosts.ghost[i].pos.x + x_offset) * CELL_WIDTH,
                ((float)state->ghosts.ghost[i].pos.y + y_offset) * CELL_HEIGHT,
                CELL_WIDTH,
                CELL_HEIGHT });
    }
    if (state->options.draw_targets)
    {
        for (auto i = 0; i < 4; i++)
        {
            SDL_SetRenderDrawColor (state->video.sdl.renderer,
                state->ghosts.ghost[i].color.r,
                state->ghosts.ghost[i].color.g,
                state->ghosts.ghost[i].color.b,
                125);
            SDL_RenderFillRect (state->video.sdl.renderer,
                &(SDL_FRect){ .x = state->ghosts.ghost[i].pos.x * CELL_WIDTH,
                    .y = state->ghosts.ghost[i].pos.y * CELL_HEIGHT,
                    .w = CELL_WIDTH,
                    .h = CELL_HEIGHT });
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
                    (SDL_FPoint){ state->ghosts.ghost[i].target.x * CELL_WIDTH
                                      - CELL_WIDTH / 2,
                                 state->ghosts.ghost[i].target.y * CELL_HEIGHT
                            + CELL_HEIGHT / 2 }
            },
                3);
        }
    }
    if (state->options.draw_ghost_cell)
    {
        for (auto i = 0; i < 4; i++)
        {
            SDL_SetRenderDrawColor (state->video.sdl.renderer,
                state->ghosts.ghost[i].color.r,
                state->ghosts.ghost[i].color.g,
                state->ghosts.ghost[i].color.b,
                125);
            SDL_RenderFillRect (state->video.sdl.renderer,
                &(SDL_FRect){
                    .x = state->ghosts.ghost[i].pos.x * CELL_WIDTH,
                    .y = state->ghosts.ghost[i].pos.y * CELL_HEIGHT,
                    .w = CELL_WIDTH,
                    .h = CELL_HEIGHT });
        }
    }
}