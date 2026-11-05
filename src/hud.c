/*
 * HUD and banners as sprites. Both tile layers scroll, so anything that has to stay put is
 * drawn with sprites on top: hearts, air bubbles, the fish count and lives, and the font tiles
 * (at their ASCII codes) for banners.
 */
#include "game.h"
#include <zgdk/ascii.h>
#include <core.h>
#include <zgdk/hud/sprite.h>

#define H_HEARTS  0   // 3 hearts
#define H_AIR     3   // 5 bubbles
#define H_FISH    8   // icon + 2 digits
#define H_LIVES   11  // pup icon + 'x' + digit
#define HUD_Y     (4 + 16)
#define SPRITE_OFF 0

static gfx_sprite *hud;   // contiguous HUD sprites, followed by the banner rows
#define TEXT(line, i) (&hud[HUD_SPRITES + (line) * TEXT_LEN + (i)])
static uint8_t shown;
static uint8_t dirty;
#define DIRTY_HUD  1
#define DIRTY_TEXT 2
static uint8_t last_hearts = 0xFF;
static uint8_t last_air = 0xFF;
static uint8_t last_fish = 0xFF;
static uint8_t last_lives = 0xFF;

static gfx_sprite *add(uint16_t x) {
    gfx_sprite s = { .y = SPRITE_OFF, .x = x, .tile = 0, .flags = SPRITE_NONE, .options = SPRITE_OPTION_NONE };
    return sprites_register(s);
}

zos_err_t hud_init(void) {
    uint8_t i, j;
    gfx_sprite *last;
    // This game's font occupies tiles 32-95; unsupported characters are blank.
    mem_set(ASCII_MAP, 32, sizeof(ASCII_MAP));
    ascii_map(' ', 64, 32);

    hud = add(6 + 16);
    for (i = 1; i < HEARTS; i++) add(6 + i * 14 + 16);
    for (i = 0; i < AIR_PIPS; i++) add(112 + i * 12 + 16);
    add(214 + 16);
    add(230 + 16);
    add(242 + 16);
    add(270 + 16);
    add(282 + 16);
    add(294 + 16);
    for (j = 0; j < TEXT_LINES; j++) {
        for (i = 0; i < TEXT_LEN; i++) last = add(0);
    }
    if (!last) return ERR_NO_MORE_MEMORY;
    hud[H_FISH].tile = SPR_FISH_ICON;
    hud[H_LIVES].tile = SPR_PUP_SIT;
    hud[H_LIVES + 1].tile = ascii_to('\\');   // the font's times sign
    dirty = DIRTY_HUD | DIRTY_TEXT;
    return ERR_SUCCESS;
}

/* The entities precede this contiguous range in the arena. */
void hud_render(void) {
    if (dirty & DIRTY_HUD) gfx_sprite_render_array(&vctx, POOL_SIZE, hud, HUD_SPRITES);
    if (dirty & DIRTY_TEXT) gfx_sprite_render_array(&vctx, POOL_SIZE + HUD_SPRITES, hud + HUD_SPRITES, TEXT_LINES * TEXT_LEN);
    dirty = 0;
}

void hud_show(uint8_t on) {
    uint8_t i;
    shown = on;
    for (i = 0; i < HUD_SPRITES; i++) hud[i].y = on ? HUD_Y : SPRITE_OFF;
    last_hearts = last_air = last_fish = last_lives = 0xFF;
    dirty |= DIRTY_HUD;
}

void hud_draw(void) {
    uint8_t i, air;
    if (!shown) return;
    if (game.hearts != last_hearts) {
        last_hearts = game.hearts;
        dirty |= DIRTY_HUD;
        for (i = 0; i < HEARTS; i++) hud[H_HEARTS + i].tile = i < game.hearts ? SPR_HEART : SPR_HEART_OFF;
    }
    // one bubble per 2 s of air, rounded up
    air = (uint8_t)((game.air + (AIR_MAX / AIR_PIPS) - 1) / (AIR_MAX / AIR_PIPS));
    if (air != last_air) {
        last_air = air;
        dirty |= DIRTY_HUD;
        for (i = 0; i < AIR_PIPS; i++) hud[H_AIR + i].tile = i < air ? SPR_AIR : SPR_AIR_OFF;
    }
    if (game.fish != last_fish) {
        last_fish = game.fish;
        dirty |= DIRTY_HUD;
        hud[H_FISH + 1].tile = ascii_to('0' + game.fish / 10);
        hud[H_FISH + 2].tile = ascii_to('0' + game.fish % 10);
    }
    if (game.lives != last_lives) {
        last_lives = game.lives;
        dirty |= DIRTY_HUD;
        hud[H_LIVES + 2].tile = ascii_to('0' + game.lives);
    }
}

void text_show(uint8_t line, const char *str, uint8_t y) {
    hud_sprite_text_t row = { TEXT(line, 0), TEXT_LEN, 12 };
    dirty |= DIRTY_TEXT;
    // Preserve this font's visual centering (12 px advance, 2 px correction).
    hud_sprite_put_s_centered(&row, str, 158, y);
}

void text_hide(uint8_t line) {
    dirty |= DIRTY_TEXT;
    hud_sprite_hide(TEXT(line, 0), TEXT_LEN);
}

void text_hide_all(void) {
    dirty |= DIRTY_TEXT;
    hud_sprite_hide(TEXT(0, 0), TEXT_LINES * TEXT_LEN);
}
