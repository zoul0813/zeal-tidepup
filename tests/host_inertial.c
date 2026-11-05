#include <zgdk/physics/platformer.h>
/* Host-side test of the shared swimming and push helpers. */
#include <stdio.h>
#include <stdint.h>
#include <zgdk/tilemap/flags.h>
#include <zgdk/physics/body.h>
#include <zgdk/physics/inertial.h>

static const char *MAP[15] = {
    "####################",
    "#..................#",
    "#..................#",
    "#~~~~~~~~~~~~~~~~~~#",
    "#~~~~~~~~~~~~~~~~~~#",
    "#~~~~~~~~~~~~~~~~~~#",
    "#~~~~~~~~~~~~~~~~~~#",
    "#~~~~~~~~~~~~~~~~~~#",
    "#~~~~~~~~~~~~~~~~~~#",
    "#~~~~~~~~~~~~~~~~~~#",
    "#~~~~~~~~~~~~~~~~~~#",
    "#~~~~~~~~~~~~~~~~~~#",
    "#~~~~~~~~~~~~~~~~~~#",
    "####################",
    "####################",
};

#include "zgdk/tilemap.h"

TILEFLAGS_DEFINE_CACHE(40);
Tilemap *_TILEMAP;
static Tilemap tilemap;
static uint8_t tiles[20 * 15];
static uint8_t flags[256];
static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)
#define WATER TF_USER1

static const inertial_config_t SWIM = { 3, 2, 40, -2, 2 };

int main(void) {
    body_t b;
    int x, y, i;
    for (y = 0; y < 15; y++) for (x = 0; x < 20; x++) tiles[y * 20 + x] = (uint8_t)MAP[y][x];
    tilemap.tiles = tiles; tilemap.rect.w = 20; tilemap.rect.h = 15; _TILEMAP = &tilemap;
    flags['#'] = TF_SOLID; flags['~'] = WATER;
    tileflags_register(flags);

    // swim right: speeds up to max_speed, then stops at the wall
    body_init(&b, 40, 120, 12, 12);
    for (i = 0; i < 30; i++) { inertial_update(&b, &SWIM, 1, 0); body_step(&b); }
    CHECK(b.vx == 40, "top speed %d", b.vx);
    for (i = 0; i < 200; i++) { inertial_update(&b, &SWIM, 1, 0); body_step(&b); }
    CHECK(body_px(&b) == 19 * 16 - 12, "stopped at wall x %d", body_px(&b));
    // diagonal clamp
    for (i = 0; i < 40; i++) { inertial_update(&b, &SWIM, -1, 1); body_step(&b); }
    CHECK(b.vx == -30 && b.vy == 30, "diagonal %d %d", b.vx, b.vy);
    // idle: drag to rest, buoyancy floats up slowly
    for (i = 0; i < 40; i++) { inertial_update(&b, &SWIM, 0, 0); body_step(&b); }
    CHECK(b.vx == 0 && b.vy == -2, "idle drift %d %d", b.vx, b.vy);
    // a kick above max_speed decays by drag, not instantly
    body_push(&b, 0, -80, 80);
    inertial_update(&b, &SWIM, 0, -1);
    CHECK(b.vy < -40 - 20, "kick keeps speed one frame: %d", b.vy);
    // glide: 2 per frame, so 40 frames is plenty to get back to max_speed
    for (i = 0; i < 40; i++) inertial_update(&b, &SWIM, 0, -1);
    CHECK(b.vy == -40, "kick decays to max %d", b.vy);
    printf(fails ? "host_motion: %d FAILED\n" : "host_motion: all passed\n", fails);
    return fails != 0;
}
