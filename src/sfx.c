/*
 * Sound effects: tables for ZGDK's sequencer (zgdk/sound/sfx.h) on voice 3.
 * Voices 0-2 belong to the ZMT music (music.c), which never writes voice 3.
 */
#include "game.h"

static const sfx_seg S_HOP[]    = { SFX_SEG(WAV_TRIANGLE, 180, 120, 3), SFX_END };
static const sfx_seg S_JUMP[]   = { SFX_SEG(WAV_SQUARE, 260, 520, 6), SFX_END };
static const sfx_seg S_SPLASH[] = { SFX_SEG(WAV_NOISE, 3000, 600, 10), SFX_END };
static const sfx_seg S_LEAP[]   = { SFX_SEG(WAV_SQUARE, 300, 900, 10), SFX_SEG(WAV_SQUARE, 900, 1200, 6), SFX_END };
static const sfx_seg S_KICK[]   = { SFX_SEG(WAV_TRIANGLE, 400, 200, 5), SFX_SEG(WAV_NOISE, 800, 400, 3), SFX_END };
static const sfx_seg S_FISH[]   = { SFX_SEG(WAV_SQUARE, 988, 988, 3), SFX_SEG(WAV_SQUARE, 1319, 1319, 3), SFX_SEG(WAV_SQUARE, 1976, 1976, 5), SFX_END };
static const sfx_seg S_HURT[]   = { SFX_SEG(WAV_SQUARE, 600, 150, 12), SFX_END };
static const sfx_seg S_BUOY[]   = { SFX_SEG(WAV_TRIANGLE, 784, 784, 5), SFX_SEG(WAV_TRIANGLE, 1047, 1047, 5), SFX_SEG(WAV_TRIANGLE, 1568, 1568, 10), SFX_END };
static const sfx_seg S_GASP[]   = { SFX_SEG(WAV_NOISE, 1200, 2400, 8), SFX_END };
static const sfx_seg S_LOST[]   = { SFX_SEG(WAV_SQUARE, 700, 500, 10), SFX_SEG(WAV_SQUARE, 500, 300, 10), SFX_SEG(WAV_SQUARE, 300, 120, 24), SFX_END };
static const sfx_seg S_SQUAWK[] = { SFX_SEG(WAV_SAWTOOTH, 1400, 900, 5), SFX_SEG(WAV_SAWTOOTH, 1300, 800, 6), SFX_END };

void game_sfx_init(void) {
    sfx_init(VOICE3, VOL_75);
}

void game_sfx_hop(void)    { sfx_play_prio(S_HOP, 0); }
void game_sfx_jump(void)   { sfx_play_prio(S_JUMP, 1); }
void game_sfx_kick(void)   { sfx_play_prio(S_KICK, 1); }
void game_sfx_splash(void) { sfx_play_prio(S_SPLASH, 1); }
void game_sfx_squawk(void) { sfx_play_prio(S_SQUAWK, 1); }
void game_sfx_leap(void)   { sfx_play_prio(S_LEAP, 2); }
void game_sfx_fish(void)   { sfx_play_prio(S_FISH, 2); }
void game_sfx_buoy(void)   { sfx_play_prio(S_BUOY, 2); }
void game_sfx_gasp(void)   { sfx_play_prio(S_GASP, 2); }
void game_sfx_hurt(void)   { sfx_play_prio(S_HURT, 3); }
void game_sfx_lost(void)   { sfx_play_prio(S_LOST, 3); }
