#include "water.h"
#include <zgdk/tilemap/flags.h>

uint8_t body_in(const body_t *body, uint8_t flags) {
    int16_t cx = PHYSICS_FP_INT(body->x) + (body->w >> 1);
    int16_t cy = PHYSICS_FP_INT(body->y) + (body->h >> 1);
    return tileflags_at(cx, cy) & flags;
}
