/* Autopilot routes, generated into <build>/maps/map_routes.c from TMX Routes layers */
#ifndef ROUTE_H
#define ROUTE_H
#include <stdint.h>

#define R_LEAP   1   // from here swim straight up and leap out of the water
#define R_SLIDE  2   // belly slide toward this point
#define R_JUMP   4   // jump on the way here
#define R_END    0xFF

typedef struct {
    uint8_t tx, ty, flags;
} waypoint_t;

extern const waypoint_t *const ROUTES[];
extern const uint8_t ROUTE_COUNT;

#endif
