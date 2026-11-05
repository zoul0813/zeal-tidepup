#include <zgdk/physics/platformer.h>
/*
 * The pup. Two animals in one body:
 *
 * On land it is slow and floppy. Holding a direction moves it in heaves: a short fast push, then
 * a slow scrunch (max_run drops every 8 frames, so platformer_update() clamps the speed down).
 * Its jump clears one tile. Holding the slide button flattens it into a belly slide: faster, and
 * on slick ice it can't stop until it hits something.
 *
 * In water it swims in 8 directions with the ZGDK's inertial_update(), and the jump button is a
 * flipper kick that bursts above swimming speed. Kicking up from 2 tiles down or more and hitting the
 * surface while still at kick speed is a breach: the pup flies about 5 tiles high, which is how
 * the high shelves are reached. A kick from just under the surface only pops it out 2 tiles.
 * Coming up slowly just floats at the surface, where it can breathe.
 *
 * One hitbox (14x12) for every pose. The sprite is a 16x16 tile, except when the pup moves
 * mostly up or down (leaping, diving): then it switches to the hardware 16x32 sprite with the
 * head up, flipped on Y to dive head first.
 */
#include "game.h"
#include "water.h"

#define PUP_W         14
#define PUP_H         12
#define KICK_PUSH     56
#define KICK_LIMIT    76
#define KICK_COOL     22
#define LEAP_MAX      (-128)     // 8 px/frame out of the water: about 5 tiles of air
#define BREACH_RUN    5          // frames of kick-speed upward swimming that make a breach: a kick
                                 // from ~1 tile down still only pops, from 2 tiles down it breaches
#define BREACH_SPEED  (-48)      // faster than plain swimming (40): only a kick gets there
#define POP_VEL       (-72)      // a pop out of the surface: ~27 px, enough for a shelf one tile up
#define INVULN        90
#define DEAD_TIME     90
#define GASP_EVERY    90         // out of air: lose a heart this often

#define M_LAND   0
#define M_SWIM   1
#define M_DEAD   2
#define M_DONE   3

static platformer_config_t land = {
    .gravity = 6,
    .max_fall = 112,
    .jump_vel = -62,           // ~19 px: one tile step
    .jump_cut = -24,
    .accel = 6,
    .decel = 4,
    .max_run = 26,
    .coyote = 5,
    .buffer = 6,
    .air_jumps = 0,
};

static const inertial_config_t SWIM = {
    .accel = 3,
    .drag = 2,
    .max_speed = 40,           // 2.5 px/frame
    .drift_y = -3,            // floats up slowly when idle
    .glide = 1,                // a kick carries ~28 frames before it's back to swimming speed
};

static const uint8_t F_SIT[] = { SPR_PUP_SIT };
static const uint8_t F_HUMP[] = { SPR_PUP_SIT, SPR_PUP_HUMP };
static const uint8_t F_SLIDE[] = { SPR_PUP_SLIDE };
static const uint8_t F_SWIM[] = { SPR_PUP_SWIM1, SPR_PUP_SWIM2 };
static const uint8_t F_UP[] = { SPR_PUP_UP1, SPR_PUP_UP2 };
static const uint8_t F_HURT[] = { SPR_PUP_HURT };
static const anim_def A_SIT = { F_SIT, 1, 1, ANIM_LOOP };
static const anim_def A_HUMP = { F_HUMP, 2, 8, ANIM_LOOP };
static const anim_def A_SLIDE = { F_SLIDE, 1, 1, ANIM_LOOP };
static const anim_def A_SWIM = { F_SWIM, 2, 10, ANIM_LOOP };
static const anim_def A_SWIM_FAST = { F_SWIM, 2, 5, ANIM_LOOP };
static const anim_def A_UP = { F_UP, 2, 8, ANIM_LOOP };
static const anim_def A_HURT = { F_HURT, 1, 1, ANIM_LOOP };

static game_entity_t *pup;
static uint8_t mode;
static uint8_t timer;
static uint8_t phase;          // galumph cycle on land
static uint8_t kick_cool;
static uint8_t invuln;
static uint8_t gasp;
static uint8_t facing;         // 0 right, SPRITE_FLIP_X left
static uint8_t was_ground;
static uint8_t rise;           // frames spent rising at kick speed: a long run up makes a breach
static uint8_t launched;       // out of the water upward: no jump-release cut until the fall
static uint8_t slide_now;      // flat on the belly this frame: bowls crabs over, ducks under gulls
static int16_t check_x, check_y;

game_entity_t *pup_entity(void) {
    return pup;
}

uint8_t pup_in_water(void) {
    return mode == M_SWIM;
}

uint8_t pup_alive(void) {
    return mode == M_LAND || mode == M_SWIM;
}

void pup_set_checkpoint(int16_t px, int16_t py) {
    check_x = px;
    check_y = py;
}

static void set_tall(uint8_t tall) {
    // 16x16 sits on the hitbox bottom; 16x32 is centered on the hitbox
    pup->entity.physics.visual.tall = tall;
    pup->entity.physics.visual.ox = -1;
    pup->entity.physics.visual.oy = tall ? -10 : -4;
}

void pup_spawn(int16_t px, int16_t py) {
    // after entity_pool_clear(&game_entities) the old slot is free: take a slot again (spawn the pup first)
    if (pup == NULL || pup->entity.physics.visual.type != E_PUP) pup = physics_entities_spawn(&game_entities, E_PUP, px, py, PUP_W, PUP_H);
    else platformer_set_pos(&pup->entity.physics.body, &pup->platformer, px, py);
    pup->entity.physics.body.vx = 0;
    pup->entity.physics.body.vy = 0;
    pup->entity.physics.visual.hidden = 0;
    pup->entity.physics.visual.flip = 0;
    set_tall(0);
    mode = body_in(&pup->entity.physics.body, TF_WATER) ? M_SWIM : M_LAND;
    timer = 0;
    phase = 0;
    kick_cool = 0;
    rise = 0;
    launched = 0;
    invuln = 0;
    gasp = 0;
    facing = 0;
    was_ground = 0;
    animated_physics_entity_play(&pup->entity, &A_SIT);
    check_x = px;
    check_y = py;
}

/* shore cleared: stop taking input, settle where it is */
void pup_freeze(void) {
    mode = M_DONE;
    pup->entity.physics.body.vx = 0;
    if (pup->entity.physics.body.vy < 0) pup->entity.physics.body.vy = 0;
}

uint8_t pup_sliding(void) {
    return mode == M_LAND && slide_now && (pup->entity.physics.body.state & BODY_ON_GROUND);
}

void pup_respawn(void) {
    pup_spawn(check_x, check_y);
    camera_set(&cam, check_x - 120, check_y - 120);
}

void pup_hurt(int16_t from_x) {
    if (invuln || !pup_alive()) return;
    if (game.hearts) game.hearts--;
    invuln = INVULN;
    game_sfx_hurt();
    // knocked back away from what hit
    pup->entity.physics.body.vx = PHYSICS_FP_INT(pup->entity.physics.body.x) + (PUP_W / 2) < from_x ? -28 : 28;
    pup->entity.physics.body.vy = mode == M_SWIM ? -16 : -48;
    if (game.hearts == 0) {
        mode = M_DEAD;
        timer = DEAD_TIME;
        game_sfx_lost();
        animated_physics_entity_play(&pup->entity, &A_HURT);
        set_tall(0);
        pup->entity.physics.body.vy = -64;
    }
}

static void splash(void) {
    effect_spawn(E_SPLASH, PHYSICS_FP_INT(pup->entity.physics.body.x) - 1, SURFACE_Y - 14);
    game_sfx_splash();
}

static void update_land(int8_t dir, uint8_t jump_held, uint8_t jump_pressed) {
    body_t *b = &pup->entity.physics.body;
    uint8_t ground = b->state & BODY_ON_GROUND;
    uint8_t sliding = (input_now & IN_SLIDE) && ground;
    uint8_t slick = ground && (tileflags_at(PHYSICS_FP_INT(b->x) + (PUP_W / 2), PHYSICS_FP_INT(b->y) + PUP_H) & TF_SLICK);
    int16_t speed;

    if (sliding || slick) {
        // belly slide: quick, and it barely slows down; on slick ice it doesn't at all
        land.max_run = sliding ? 38 : 30;
        land.accel = slick ? 1 : 3;
        land.decel = slick ? 0 : 1;
        if (slick && b->vx == 0 && dir == 0) land.decel = 4;
    } else if (ground) {
        // galumph: a fast heave, then a slow scrunch
        land.accel = 6;
        land.decel = 4;
        land.max_run = (phase & 8) ? 6 : 26;
        if (dir) {
            if ((phase & 15) == 0) game_sfx_hop();
            phase++;
        } else {
            phase = 0;
        }
    } else {
        // air: keep the momentum of a leap, steer a little
        land.accel = 2;
        land.decel = 0;
        speed = b->vx < 0 ? -b->vx : b->vx;
        land.max_run = speed > 26 ? speed : 26;
    }
    slide_now = sliding || (slick && b->vx);
    // slick ice ignores the pad while sliding
    if (slick && b->vx) dir = 0;

    // a leap out of the water isn't a jump: letting go of the button mustn't cut it short
    if (launched && b->vy >= 0) launched = 0;
    land.jump_cut = launched ? -128 : -24;
    if (platformer_update(b, &land, &pup->platformer, dir, jump_held, jump_pressed)) {
        game_sfx_jump();
        phase = 0;
    }
    platformer_move(b, &land);

    if (body_in(b, TF_WATER)) {
        mode = M_SWIM;
        // the water swallows most of a fall
        b->vy >>= 1;
        if (b->vy > 8) splash();
        return;
    }

    ground = b->state & BODY_ON_GROUND;
    if (ground && !was_ground && b->vy == 0) game_sfx_hop();
    was_ground = ground;

    if (b->vx > 4) facing = 0;
    else if (b->vx < -4) facing = SPRITE_FLIP_X;

    if (!ground) {
        if (b->vy < -24 || b->vy > 48) {
            set_tall(1);
            pup->entity.physics.visual.flip = b->vy > 0 ? (facing | SPRITE_FLIP_Y) : facing;
            animated_physics_entity_play(&pup->entity, &A_UP);
        } else {
            set_tall(0);
            pup->entity.physics.visual.flip = facing;
            animated_physics_entity_play(&pup->entity, &A_HUMP);
        }
    } else {
        set_tall(0);
        pup->entity.physics.visual.flip = facing;
        if (sliding || (slick && b->vx)) animated_physics_entity_play(&pup->entity, &A_SLIDE);
        else if (dir) animated_physics_entity_play(&pup->entity, &A_HUMP);
        else animated_physics_entity_play(&pup->entity, &A_SIT);
    }
}

static void update_swim(int8_t dir, uint8_t jump_pressed) {
    body_t *b = &pup->entity.physics.body;
    int8_t dy = 0;
    int16_t old_y = b->y;
    int16_t ax, ay;

    if (input_now & IN_UP) dy = -1;
    else if (input_now & IN_DOWN) dy = 1;

    inertial_update(b, &SWIM, dir, dy);
    if (jump_pressed && !kick_cool) {
        // kick the way the pad points, or straight ahead
        int16_t kx = dir ? dir : (dy ? 0 : (facing ? -1 : 1));
        body_push(b, kx * KICK_PUSH, dy * KICK_PUSH, KICK_LIMIT);
        kick_cool = KICK_COOL;
        game_sfx_kick();
        effect_spawn(E_BUBBLE, PHYSICS_FP_INT(b->x) + (facing ? 12 : -4), PHYSICS_FP_INT(b->y));
    }
    body_step(b);
    if (b->vy < BREACH_SPEED) {
        if (rise < 255) rise++;
    } else {
        rise = 0;
    }

    if (!body_in(b, TF_WATER)) {
        if (b->vy < -32) {
            // out of the surface fast. Still riding a kick up from below: a breach, about 5 tiles
            // of air. A kick from just under the surface or plain swimming only pops out (2 tiles)
            if (rise >= BREACH_RUN) {
                b->vy = LEAP_MAX;
                game_sfx_leap();
            } else {
                if (b->vy > POP_VEL) b->vy = POP_VEL;
                game_sfx_jump();
            }
            launched = 1;
            rise = 0;
            mode = M_LAND;
            was_ground = 0;
            splash();
            return;
        }
        if (b->state & BODY_ON_GROUND || b->vy >= 0) {
            // pushed out sideways onto a shelf or beach
            mode = M_LAND;
            return;
        }
        // too slow to jump out: bob at the surface
        b->y = old_y;
        b->vy = 0;
    }

    if (b->vx > 4) facing = 0;
    else if (b->vx < -4) facing = SPRITE_FLIP_X;
    ax = b->vx < 0 ? -b->vx : b->vx;
    ay = b->vy < 0 ? -b->vy : b->vy;
    if (ay > ax + 8) {
        set_tall(1);
        pup->entity.physics.visual.flip = b->vy > 0 ? (facing | SPRITE_FLIP_Y) : facing;
        animated_physics_entity_play(&pup->entity, &A_UP);
    } else {
        set_tall(0);
        pup->entity.physics.visual.flip = facing;
        animated_physics_entity_play(&pup->entity, (ax > 40 || ay > 40) ? &A_SWIM_FAST : &A_SWIM);
    }
}

void pup_update(void) {
    body_t *b = &pup->entity.physics.body;
    int8_t dir = 0;
    uint8_t jump_held = (input_now & IN_JUMP) != 0;
    uint8_t jump_pressed = (input_pressed & IN_JUMP) != 0;
    int16_t head;

    if (input_now & IN_RIGHT) dir = DIRECTION_RIGHT;
    else if (input_now & IN_LEFT) dir = DIRECTION_LEFT;
    if (kick_cool) kick_cool--;

    switch (mode) {
        case M_DONE:
            platformer_move(b, &land);
            pup->entity.physics.visual.sprite->tile = anim_tick(&pup->entity.anim);
            return;
        case M_DEAD:
            // tumble down through everything
            b->vy += 4;
            if (b->vy > 64) b->vy = 64;
            b->y += b->vy;
            pup->entity.physics.visual.hidden = (timer & 4) != 0;
            if (--timer == 0) {
                pup->entity.physics.visual.hidden = 1;
                game.state = GS_DEAD;
            }
            return;
        case M_LAND:
            update_land(dir, jump_held, jump_pressed);
            break;
        case M_SWIM:
            update_swim(dir, jump_pressed);
            break;
    }

    // air: runs out with the head under water, refills at the surface or on land
    head = PHYSICS_FP_INT(b->y) + 2;
    if (mode == M_SWIM && head >= SURFACE_Y) {
        if (game.air) game.air--;
        if (game.air == 0 && ++gasp >= GASP_EVERY) {
            gasp = 0;
            game_sfx_gasp();
            pup_hurt(PHYSICS_FP_INT(b->x));
        }
    } else {
        gasp = 0;
        game.air = game.air + 8 > AIR_MAX ? AIR_MAX : game.air + 8;
    }
    if (mode == M_SWIM && (frame_count & 63) == 0 && head >= SURFACE_Y) {
        effect_spawn(E_BUBBLE, PHYSICS_FP_INT(b->x) + (facing ? 0 : 10), PHYSICS_FP_INT(b->y) - 6);
    }

    if ((b->state & BODY_TOUCHING_TRIGGER) && pup_alive()) pup_hurt(PHYSICS_FP_INT(b->x) + (facing ? -8 : 24));

    if (invuln) {
        invuln--;
        pup->entity.physics.visual.hidden = (invuln & 4) != 0;
        if (!invuln) pup->entity.physics.visual.hidden = 0;
    }
    pup->entity.physics.visual.sprite->tile = anim_tick(&pup->entity.anim);
}
