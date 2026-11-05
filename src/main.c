/*
 * Tidepup: a seal pup brings fish home across four shores. Slow and floppy on land, fast in the
 * water, and a hard swim up through the surface throws it high onto the ice.
 * Side-scroller for Zeal 8-bit OS, built on ZGDK.
 */
#include <zos_vfs.h>
#include <stdint.h>
#include <zos_errors.h>
#include <zos_sys.h>
#include <zos_video.h>
#include "game.h"
#include "controls.h"
#include "entities.h"
#include "assets.h"
#include "zgdk/tilemap/scroll.h"
#include <zgdk/sprites.h>

#ifndef START_SHORE
#define START_SHORE 0
#endif
#define BANNER_FRAMES   150
#define CLEAR_FRAMES    220
#define END_FRAMES      360
/* Shore 1: the ice at tiles 56-60 is the first shelf too tall to hop onto, so say how to breach */
#define HINT_SHORE      0
#define HINT_X0         38      // pup center tile range: the water in front of that shelf
#define HINT_X1         55

gfx_context vctx;
game_t game;
entity_pool_t game_entities;
uint16_t input_now;
uint16_t input_pressed;
uint8_t frame_count;
static uint16_t input_last;
static uint8_t hint_shown;
static game_entity_t pool[POOL_SIZE];
static gfx_sprite arena[ARENA_SIZE];
#ifdef AUTOPILOT
static uint8_t autopilot_on;
#endif

static void fatal(zos_err_t err, const char *what) {
    // printf would pull in ~2.6 KB of formatting code
    uint16_t len = 0;
    char code[4] = { ' ', '0' + (err / 10) % 10, '0' + err % 10, '\n' };
    ioctl(DEV_STDOUT, CMD_RESET_SCREEN, NULL);
    while (what[len]) len++;
    write(DEV_STDOUT, what, &len);
    len = sizeof(code);
    write(DEV_STDOUT, code, &len);
    exit(err);
}

static void read_input(void) {
    input_now = input_read();
#ifdef AUTOPILOT
    if (autopilot_on) input_now = (input_now & (IN_START | IN_QUIT)) | autopilot_input();
#endif
    input_pressed = input_now & ~input_last;
    input_last = input_now;
}

static void frame_end(void) {
    world_frame();
    physics_entities_sync(&game_entities);
    hud_draw();
    gfx_wait_vblank(&vctx);
    world_scroll();
    gfx_sprite_render_array(&vctx, 0, arena, POOL_SIZE);
    hud_render();
    gfx_wait_end_vblank(&vctx);
    bgm_tick();
    sfx_tick();
    frame_count++;
}

static void init(void) {
    zos_err_t err;

    if (input_init(1) != ERR_SUCCESS) input_init(0);

    gfx_enable_screen(0);
    err = gfx_initialize(ZVB_CTRL_VID_MODE_GFX_320_8BIT, &vctx);
    if (err) fatal(err, "gfx");
    err = sprites_clear(&vctx);   // a previous program may have left sprites in the table
    if (err) fatal(err, "sprites");
    err = zar_file_open(PACK_FILE, &pack);   // tidepup.zar, next to the .bin
    if (err) fatal(err, PACK_FILE);
    err = load_palette(&vctx);
    if (err) fatal(err, "palette");
    err = load_tileset(&vctx);
    if (err) fatal(err, "tileset");
    tilemap_fill(&vctx, PLAY_LAYER, TILE_EMPTY, 0, 0, 80, 40);

    sprites_register_arena(arena, ARENA_SIZE);
    err = sprite_entities_init(&game_entities, pool, POOL_SIZE, sizeof(*pool));
    if (err) fatal(err, "sprites");
    err = hud_init();   // after the entities: higher sprite slots draw on top of lower ones
    if (err) fatal(err, "hud");
    err = world_init();
    if (err) fatal(err, "world");

    sound_init_hardware();       // resets every voice, so before the tracker and the effects
    err = bgm_init();   // after load_tileset(): the patterns reuse its load buffer
    if (err) fatal(err, "music");
    game_sfx_init();
    rand8_seed(0x5EA1);

    gfx_enable_screen(1);
}

static void deinit(void) {
    bgm_stop();
    sound_deinit();
    zar_file_close(&pack);
    tilemap_scroll(BG_LAYER, 0, 0);
    tilemap_scroll(PLAY_LAYER, 0, 0);
    (void)sprites_clear(&vctx);   // best-effort cleanup before returning to ZOS
    ioctl(DEV_STDOUT, CMD_RESET_SCREEN, NULL);
}

/* Title over shore 1, the camera drifting along the ice. Returns 0 to quit. */
static uint8_t title(void) {
    uint16_t seed = 0;
    int8_t pan = 1;

    entity_pool_clear(&game_entities);
    game.shore = 0;
    world_load(0);
    actors_reset();
    camera_set(&cam, 0, 96);
    hud_show(0);
    input_flush();
    controls_menu_begin();
    text_hide_all();
    text_show(0, "TIDEPUP", 36);
    text_show(1, PROMPT_START, 92);
    text_show(2, CONTROL_PROMPT("Z SWIM KICK  X SLIDE", PAD_B " SWIM KICK  " PAD_A " SLIDE"), 200);
    input_last = 0;
    bgm_play(BGM_TITLE);

    while (1) {
        read_input();
        if (controls_menu_update()) {
            text_show(1, PROMPT_START, 92);
            text_show(2, CONTROL_PROMPT("Z SWIM KICK  X SLIDE", PAD_B " SWIM KICK  " PAD_A " SLIDE"), 200);
        }
        seed++;
        if (input_pressed & IN_START) break;
        if (input_pressed & IN_QUIT) return 0;
#ifdef AUTOPILOT
        if (seed == 120) break;
#endif
        cam.x += pan;
        if (cam.x >= cam.max_x) pan = -1;
        else if (cam.x <= 0) pan = 1;
        actors_update();
        frame_end();
    }
    text_hide_all();
    rand8_seed(seed | 1);
    return 1;
}

/* The breach hint on text line 2 while the pup is in front of the first tall shelf */
static void hint_update(void) {
    game_entity_t *e = pup_entity();
    uint8_t want = 0;
    if (e && game.shore == HINT_SHORE && game.state == GS_PLAY && game.timer == 0) {
        int16_t tx = (PHYSICS_FP_INT(e->entity.physics.body.x) + (e->entity.physics.body.w >> 1)) >> 4;
        want = tx >= HINT_X0 && tx <= HINT_X1;
    }
    if (want == hint_shown) return;
    hint_shown = want;
    if (want) text_show(2, CONTROL_PROMPT("DIVE, THEN UP + Z", "DIVE, THEN " PAD_UP " + " PAD_B), 40);
    else text_hide(2);
}

static void shore_start(void) {
    const shore_t *s = &SHORES[game.shore];
    char head[] = "SHORE 0";

    entity_pool_clear(&game_entities);
    world_load(game.shore);
    actors_reset();
    pup_spawn((int16_t)s->start_tx * 16 + 1, (int16_t)s->start_ty * 16 + 4);
    game.fish = 0;
    game.hearts = HEARTS;
    game.air = AIR_MAX;
    head[6] = '1' + game.shore;
    text_show(0, head, 64);
    text_show(1, s->name, 84);
    game.state = GS_PLAY;
    game.timer = BANNER_FRAMES;
    hint_shown = 0;
    hud_show(1);
    bgm_play(BGM_SHORE);
#ifdef AUTOPILOT
    autopilot_reset();
#endif
}

static void pause(void) {
    text_show(2, "PAUSED", 110);
    bgm_pause();
    input_flush();
    input_last = 0;
    do {
        read_input();
        frame_end();
    } while (!(input_pressed & IN_START));
    text_hide(2);
    hint_shown = 0;   // shows again next frame if the pup is still in front of the shelf
    bgm_resume();
}

static void count_line(char *buf, uint8_t got, uint8_t of) {
    // "FISH 12/15"
    buf[5] = '0' + got / 10;
    buf[6] = '0' + got % 10;
    buf[8] = '0' + of / 10;
    buf[9] = '0' + of % 10;
}

static void play(void) {
    char line[] = "FISH 00/00";

    game.shore = START_SHORE;
    game.lives = LIVES;
    game.fish_total = 0;
#ifdef AUTOPILOT
    autopilot_on = 1;
#endif
    shore_start();

    while (1) {
        read_input();
        if (input_pressed & IN_START) pause();
        if (input_pressed & IN_QUIT) break;

        if (game.state == GS_PLAY || game.state == GS_CLEAR) pup_update();
        actors_update();

        if (game.timer && --game.timer == 0 && game.state == GS_PLAY) {
            text_hide(0);
            text_hide(1);
        }
        hint_update();

        switch (game.state) {
            case GS_CLEAR:
                if (game.timer == 0) {
                    // just reached the mother
                    pup_freeze();
                    game.timer = CLEAR_FRAMES;
                    text_show(0, "HOME!", 64);
                    count_line(line, game.fish, SHORES[game.shore].fish);
                    text_show(1, line, 84);
                    bgm_play(BGM_CLEAR);
                    game.state = GS_WIN;   // GS_WIN with a timer = between shores
                }
                break;
            case GS_WIN:
                if (game.timer == 0) {
                    if (++game.shore >= SHORE_COUNT) goto won;
                    shore_start();
                }
                break;
            case GS_DEAD:
                if (game.lives) game.lives--;
                if (game.lives == 0) {
                    game.state = GS_OVER;
                    game.timer = END_FRAMES;
                    text_show(0, "GAME OVER", 64);
                    bgm_play(BGM_OVER);
                } else {
                    game.hearts = HEARTS;
                    game.air = AIR_MAX;
                    pup_respawn();
                    game.state = GS_PLAY;
#ifdef AUTOPILOT
                    autopilot_respawn();
#endif
                }
                break;
            case GS_OVER:
                if (game.timer == 0) goto done;
                break;
        }
        frame_end();
    }
    goto done;
won:
    entity_pool_clear(&game_entities);
    hud_show(0);
    text_show(0, "ALL HOME!", 56);
    count_line(line, (uint8_t)game.fish_total, 0);
    line[8] = 0;   // "FISH 12" without the total
    text_show(1, line, 80);
    text_show(2, "THE COLONY IS FED", 120);
    bgm_play(BGM_CLEAR);
    for (game.timer = END_FRAMES; game.timer; game.timer--) {
        read_input();
        actors_update();
        frame_end();
    }
done:
    entity_pool_clear(&game_entities);
    text_hide_all();
    hud_show(0);
    bgm_stop();
#ifdef AUTOPILOT
    autopilot_on = 0;
#endif
    frame_end();
    input_flush();
}

int main(void) {
    init();
    while (title()) play();
    deinit();
    return 0;
}
