#include <stdint.h>
#include <zos_errors.h>
#include <zvb_gfx.h>
/* only the ZGDK headers used: <zgdk.h> pulls rect.h/ascii.h/music.h, whose non-inline static
 * functions get compiled into every .c that includes them */
#include "zgdk/input.h"
#include "zgdk/sprites.h"
#include "zgdk/tilemap.h"
#include "zgdk/utils/rand.h"
#include "zgdk/sound/sounds.h"
#include "zgdk/sound/tracker.h"
#include <zgdk/physics/body.h>
#include <zgdk/tilemap/flags.h>
#include <zgdk/sound/sfx.h>
#include <zgdk/physics/inertial.h>
#include <zgdk/camera.h>
#include "stream.h"
#include "gfx.h"
#include "entities.h"

#ifndef GAME_H
#define GAME_H

#define SCREEN_W       320
#define SCREEN_H       240

#define BG_LAYER       0   // sky and sea, half speed sideways, full speed up and down
#define PLAY_LAYER     1   // shore, streamed from the world buffer

#define WORLD_W        112
#define WORLD_H        24
#define WATER_ROW      12
#define SURFACE_Y      (WATER_ROW * 16)
#define SHORE_COUNT    4

#define TF_WATER       TF_USER1
#define TF_SLICK       TF_USER2

#define POOL_SIZE      24  // pup + fish, crabs, jellies, gulls, buoys, mother, splashes, bubbles
#define HUD_SPRITES    14
#define TEXT_LINES     3
#define TEXT_LEN       20
#define ARENA_SIZE     (POOL_SIZE + HUD_SPRITES + TEXT_LINES * TEXT_LEN)

/* entity types */
#define E_PUP     0
#define E_FISH    1
#define E_CRAB    2
#define E_JELLY   3
#define E_GULL    4
#define E_BUOY    5
#define E_MOM     6
#define E_SPLASH  7
#define E_BUBBLE  8
#define E_STAR    9

/* spawn types in the level data (tools/pack_maps.py) */
#define SP_FISH   1
#define SP_CRAB   2
#define SP_JELLY  3
#define SP_GULL   4
#define SP_BUOY   5
#define SP_MOM    6
#define MAX_SPAWNS 40   // per shore (tools/pack_maps.py checks it); the actors keep two arrays this long

/* input */
#define IN_LEFT   BUTTON_LEFT
#define IN_RIGHT  BUTTON_RIGHT
#define IN_UP     BUTTON_UP
#define IN_DOWN   BUTTON_DOWN
#define IN_JUMP   BUTTON_B                 // Z / Space, SNES B: jump, flipper kick
#define IN_SLIDE  BUTTON_A                 // X, SNES A: belly slide
#define IN_START  BUTTON_START             // Enter, SNES Start
#define IN_QUIT   BUTTON_SELECT            // Right Shift, SNES Select

#define HEARTS     3
#define LIVES      3
#define AIR_MAX    900     // frames of air under water (15 s)
#define AIR_PIPS   5

typedef struct {
    uint8_t type, tx, ty, arg;
} spawn_t;

typedef struct {
    uint8_t spawn_count;      // separate shore<N>.spn records in tidepup.zar, read into shore_spawns
    uint8_t start_tx, start_ty;
    uint8_t fish;
    const char *name;
} shore_t;

typedef struct {
    uint8_t shore;        // 0 based
    uint8_t hearts;
    uint8_t lives;
    uint16_t air;
    uint8_t fish;         // this shore
    uint16_t fish_total;  // whole run
    uint8_t state;        // GS_*
    uint16_t timer;
} game_t;

#define GS_PLAY     0
#define GS_CLEAR    1
#define GS_DEAD     2
#define GS_OVER     3
#define GS_WIN      4

extern gfx_context vctx;
extern game_t game;
extern camera_t cam;
extern uint16_t input_now;
extern uint16_t input_pressed;
extern uint8_t frame_count;

/* map_data.c (generated in the build directory) */
extern const shore_t SHORES[SHORE_COUNT];
extern spawn_t shore_spawns[MAX_SPAWNS];   // the current shore's, read by world_load()

/* world.c */
zos_err_t world_init(void);
void world_load(uint8_t shore);
void world_frame(void);           // camera + streaming, before the vblank
void world_scroll(void);          // hardware scroll registers + shimmer, inside the vblank

/* pup.c */
void pup_spawn(int16_t px, int16_t py);
void pup_update(void);
void pup_hurt(int16_t from_x);
void pup_set_checkpoint(int16_t px, int16_t py);
void pup_respawn(void);
void pup_freeze(void);
uint8_t pup_in_water(void);
uint8_t pup_sliding(void);
uint8_t pup_alive(void);
game_entity_t *pup_entity(void);

/* actors.c */
void actors_reset(void);
void actors_update(void);
void effect_spawn(uint8_t type, int16_t px, int16_t py);

/* hud.c */
zos_err_t hud_init(void);
void hud_render(void);   // changed HUD/banner ranges, during vblank
void hud_draw(void);
void hud_show(uint8_t on);
void text_show(uint8_t line, const char *str, uint8_t y);
void text_hide(uint8_t line);
void text_hide_all(void);

/* audio: the music's pattern buffers sit at the start of asset_work (assets.h), the world after them */
#define MUSIC_PATTERNS 3
#define MUSIC_RAM      (MUSIC_PATTERNS * 772)   // sizeof(pattern_t)
#define BGM_NONE     0xFF
#define BGM_TITLE    0
#define BGM_SHORE    1
#define BGM_CLEAR    2
#define BGM_OVER     3
zos_err_t bgm_init(void);
void bgm_play(uint8_t song);
void bgm_stop(void);
void bgm_tick(void);
void bgm_pause(void);
void bgm_resume(void);
void game_sfx_init(void);
void game_sfx_hop(void);
void game_sfx_jump(void);
void game_sfx_splash(void);
void game_sfx_leap(void);
void game_sfx_kick(void);
void game_sfx_fish(void);
void game_sfx_hurt(void);
void game_sfx_buoy(void);
void game_sfx_gasp(void);
void game_sfx_lost(void);
void game_sfx_squawk(void);

#ifdef AUTOPILOT
uint16_t autopilot_input(void);
void autopilot_reset(void);
void autopilot_respawn(void);
#endif

#endif
