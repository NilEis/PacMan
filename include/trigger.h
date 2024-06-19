#ifndef TRIGGER_H
#define TRIGGER_H
#include "typedefs.h"
#include <stdint.h>

static constexpr uint32_t TRIGGER_DISABLED = UINT32_MAX;

static void trigger_start (const state_t *state, trigger_t *trigger)
{
    trigger->tick = state->last_ticks;
}

static void trigger_start_after (
    const state_t *state, trigger_t *trigger, const uint32_t ticks)
{
    trigger->tick = state->last_ticks + ticks;
}

static void trigger_stop (trigger_t *trigger)
{
    trigger->tick = TRIGGER_DISABLED;
}

static uint32_t trigger_ticks_since_trigger (
    const state_t *state, const trigger_t *trigger)
{
    if (state->last_ticks >= trigger->tick)
    {
        return state->last_ticks - trigger->tick;
    }
    return TRIGGER_DISABLED;
}

static bool trigger_triggered (const state_t *state, const trigger_t *trigger)
{
    return state->last_ticks >= trigger->tick;
}

static bool trigger_now (const state_t *state, const trigger_t *trigger)
{
    return state->last_ticks == trigger->tick;
}

static bool trigger_between (const trigger_t *trigger,
    const state_t *state,
    const uint32_t begin,
    const uint32_t end)
{
    if (trigger->tick != TRIGGER_DISABLED)
    {
        const auto ticks = trigger_ticks_since_trigger (state, trigger);
        return (ticks >= begin) && (ticks < end);
    }
    return false;
}

static bool trigger_after (
    const trigger_t *trigger, const state_t *state, const uint32_t ticks)
{
    return trigger->tick != TRIGGER_DISABLED
        && ticks <= trigger_ticks_since_trigger (state, trigger);
}

static bool trigger_after_once (
    const trigger_t *trigger, const state_t *state, const uint32_t ticks)
{
    return trigger->tick != TRIGGER_DISABLED
        && ticks == trigger_ticks_since_trigger (state, trigger);
}

#endif // TRIGGER_H
