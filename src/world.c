#include <zgdk/tilemap/flags.h>
/*
 * The shore: one 112x24 world buffer, read from tidepup.zar (shore<N>.ztm, assets/shores/shore<N>.tmx)
 * when the shore starts and streamed into layer 1 by the shared core (stream.h). Layer 0 is drawn once at startup: sky bands down to the
 * waterline, then the sea getting darker with depth. It scrolls at half speed sideways but at full
 * speed up and down, so its waterline always sits behind the playfield's.
 *
 * Water on layer 1 is a see-through tile with the TF_WATER flag, so the backdrop shows the sea
 * and collision still knows where it is. The surface shimmer is two palette entries swapped.
 */
#include <core.h>
#include "game.h"
#include "assets.h"

TILEFLAGS_DEFINE_CACHE(24);

camera_t cam;
static uint8_t *world;               // WORLD_W * WORLD_H tiles, inside asset_work (assets.h)
spawn_t shore_spawns[MAX_SPAWNS];
static tilemap_stream_t stream;
static Tilemap tilemap;
static uint8_t row_buf[80];
static uint8_t shimmer;

static const uint8_t TILE_FLAGS[256] = { TILE_FLAGS_INIT };

/* backdrop rows: 12 of sky (the bottom ones carry far icebergs), then the sea */
static const uint8_t SKY_ROWS[WATER_ROW] = {
    BG_SKY0, BG_SKY0, BG_SKY01, BG_SKY1, BG_SKY1, BG_SKY12,
    BG_SKY2, BG_SKY2, BG_SKY23, BG_SKY3, BG_SKY3, BG_SKY3,
};
static const uint8_t SEA_ROWS[WORLD_H - WATER_ROW] = {
    BG_SURFACE, BG_SEA0, BG_SEA01, BG_SEA1, BG_SEA12, BG_SEA2,
    BG_SEA23, BG_SEA3, BG_SEA34, BG_SEA4, BG_SEA45, BG_SEA5,
};
/* far icebergs along the horizon, every 80 columns: start column, width */
static const uint8_t BERGS[][2] = { { 4, 6 }, { 17, 3 }, { 29, 8 }, { 45, 4 }, { 58, 7 }, { 71, 3 } };

static void backdrop(void) {
    uint8_t y, i, x;
    for (y = 0; y < 40; y++) {
        uint8_t t;
        if (y < WATER_ROW) t = SKY_ROWS[y];
        else if (y < WORLD_H) t = SEA_ROWS[y - WATER_ROW];
        else t = BG_SEA5;
        for (x = 0; x < 80; x++) row_buf[x] = t;
        if (y == WATER_ROW + 1) {
            // light rays just under the surface
            for (x = 3; x < 80; x += 11) row_buf[x] = (x & 1) ? BG_RAY1 : BG_RAY0;
        } else if (y >= WATER_ROW - 2 && y < WATER_ROW) {
            // icebergs: two rows, a cap and a body
            for (i = 0; i < sizeof(BERGS) / 2; i++) {
                uint8_t x0 = BERGS[i][0], w = BERGS[i][1];
                if (y == WATER_ROW - 2) {
                    row_buf[x0 + 1] = BG_BERG_L;
                    for (x = x0 + 2; x < x0 + w - 1; x++) row_buf[x] = BG_BERG_LIT;
                    row_buf[x0 + w - 1] = BG_BERG_R;
                } else {
                    row_buf[x0] = BG_BERG_L;
                    for (x = x0 + 1; x < x0 + w; x++) row_buf[x] = x == x0 + 1 ? BG_BERG_LIT : BG_BERG;
                    row_buf[x0 + w] = BG_BERG_R;
                }
            }
        }
        gfx_tilemap_load(&vctx, row_buf, 80, BG_LAYER, 0, y);
    }
}

/* Call after load_tileset(): the world is the end of its load buffer */
zos_err_t world_init(void) {
    // music patterns first, then the world tiles
    world = asset_work + MUSIC_RAM;
    backdrop();
    tileflags_register(TILE_FLAGS);
    // walls at both ends of the world and under it; open sky above
    tileflags_set_outside(TF_SOLID);
    return ERR_SUCCESS;
}

void world_load(uint8_t shore) {
    const shore_t *s = &SHORES[shore];
    char name[] = "shore0.ztm";
    zar_entry_t file;
    uint16_t size = WORLD_W * WORLD_H, spawns = (uint16_t)s->spawn_count * sizeof(spawn_t);

    name[5] = '0' + shore;
    if (zar_entry_from_name(&pack, name, &file) != ERR_SUCCESS ||
        zar_entry_read_at(&pack, &file, 0, world, &size) != ERR_SUCCESS || size != WORLD_W * WORLD_H) {
        mem_set(world, TILE_EMPTY, WORLD_W * WORLD_H);   // unreadable: an empty shore
        mem_set(shore_spawns, 0, sizeof(shore_spawns));
    } else {
        name[7] = 's'; name[8] = 'p'; name[9] = 'n';
        if (zar_entry_from_name(&pack, name, &file) != ERR_SUCCESS ||
            zar_entry_read_at(&pack, &file, 0, shore_spawns, &spawns) != ERR_SUCCESS ||
            spawns != (uint16_t)s->spawn_count * sizeof(spawn_t))
            mem_set(shore_spawns, 0, sizeof(shore_spawns));
    }
    camera_init_view(&cam, WORLD_W * 16, WORLD_H * 16, SCREEN_W, SCREEN_H);
    cam.dz_left = 120;
    cam.dz_right = 168;
    cam.dz_top = 80;
    cam.dz_bottom = 128;
    cam.lead = 24;
    cam.max_step = 5;
    camera_set(&cam, (int16_t)s->start_tx * 16 - 120, (int16_t)s->start_ty * 16 - 120);
    stream_init(&vctx, &stream, &tilemap, world, WORLD_W, WORLD_H, PLAY_LAYER, &cam);
}

void world_frame(void) {
    game_entity_t *pup = pup_entity();
    if (pup) {
        const body_t *body = &pup->entity.physics.body;
        camera_follow_target(&cam, PHYSICS_FP_INT(body->x), PHYSICS_FP_INT(body->y), body->w, body->h,
                             body->vx < 0 ? -1 : body->vx != 0);
    }
    tilemap_stream_update(&vctx, &stream, cam.x, cam.y);
    entity_view_set(cam.x, cam.y);
}

/* surface shimmer colors, swapped every 12 frames (palette 12-13) */
static const uint16_t SHIMMER[2][2] = { { 0xD7BF, 0x867D }, { 0x867D, 0xD7BF } };

void world_scroll(void) {
    camera_scroll_layer(&cam, PLAY_LAYER, 0, 0);
    camera_scroll_layer(&cam, BG_LAYER, 1, 0);
    if (++shimmer == 12) {
        static uint8_t phase;
        shimmer = 0;
        phase ^= 1;
        gfx_palette_load(&vctx, (void *)SHIMMER[phase], 4, 12);
    }
}
