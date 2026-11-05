#ifndef TIDEPUP_WATER_H
#define TIDEPUP_WATER_H
#include <zgdk/physics/body.h>

/* Query the tile at the hitbox center, not its top-left corner. */
uint8_t body_in(const body_t *body, uint8_t flags);
#endif
