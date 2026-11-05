/* Game-owned physics checks for tidepup; moved from common host_motion. */
#include <stdio.h>
#include <stdint.h>
#include <zgdk/tilemap/flags.h>
#include <zgdk/physics/body.h>
#include "water.h"

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

int main(void) {
    body_t b;
    int x, y;
    for (y = 0; y < 15; y++) for (x = 0; x < 20; x++) tiles[y * 20 + x] = (uint8_t)MAP[y][x];
    tilemap.tiles = tiles; tilemap.rect.w = 20; tilemap.rect.h = 15; _TILEMAP = &tilemap;
    flags['#'] = TF_SOLID; flags['~'] = WATER;
    tileflags_register(flags);

    body_init(&b, 40, 120, 12, 12);
    CHECK(body_in(&b, WATER), "starts in water");
    body_set_pos(&b, 40, 20);
    CHECK(!body_in(&b, WATER), "dry above the surface");

    printf(fails ? "host_water: %d FAILED\n" : "host_water: all passed\n", fails);
    return fails != 0;
}
