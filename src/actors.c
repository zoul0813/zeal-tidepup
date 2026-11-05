#include <zgdk/physics/platformer.h>
/*
 * Everything on the shore besides the pup. The pup can swim back the way it came, so actors
 * aren't spawned once in order: each spawn in the shore's list is live only while its column is
 * near the camera, and is freed (and later respawned at home) when it drifts out of range.
 * Fish that were eaten and buoys that were lit remember it in `done`.
 */
#include "game.h"

#define WINDOW        48       // px beyond the screen edges where actors stay alive
#define FISH_SWIM     12
#define CRAB_SPEED    8
#define GULL_SPEED    16
#define GULL_RANGE    72
#define GULL_DIVE     40
#define GULL_SIGHT    80

static const uint8_t F_FISH[] = { SPR_FISH1, SPR_FISH2 };
static const uint8_t F_CRAB[] = { SPR_CRAB1, SPR_CRAB2 };
static const uint8_t F_JELLY[] = { SPR_JELLY1, SPR_JELLY2 };
static const uint8_t F_GULL[] = { SPR_GULL1, SPR_GULL2 };
static const uint8_t F_GULL_DIVE[] = { SPR_GULL_DIVE };
static const uint8_t F_BUOY[] = { SPR_BUOY };
static const uint8_t F_BUOY_ON[] = { SPR_BUOY_ON };
static const uint8_t F_MOM[] = { SPR_MOM1, SPR_MOM1, SPR_MOM2 };
static const uint8_t F_SPLASH[] = { SPR_SPLASH1, SPR_SPLASH2 };
static const uint8_t F_BUBBLE[] = { SPR_BUBBLE };
static const uint8_t F_STAR[] = { SPR_STAR };
static const anim_def A_FISH = { F_FISH, 2, 12, ANIM_LOOP };
static const anim_def A_CRAB = { F_CRAB, 2, 9, ANIM_LOOP };
static const anim_def A_JELLY = { F_JELLY, 2, 20, ANIM_LOOP };
static const anim_def A_GULL = { F_GULL, 2, 8, ANIM_LOOP };
static const anim_def A_GULL_DIVE = { F_GULL_DIVE, 1, 1, ANIM_LOOP };
static const anim_def A_BUOY = { F_BUOY, 1, 1, ANIM_LOOP };
static const anim_def A_BUOY_ON = { F_BUOY_ON, 1, 1, ANIM_LOOP };
static const anim_def A_MOM = { F_MOM, 3, 20, ANIM_LOOP };
static const anim_def A_SPLASH = { F_SPLASH, 2, 6, 0 };
static const anim_def A_BUBBLE = { F_BUBBLE, 1, 1, ANIM_LOOP };
static const anim_def A_STAR = { F_STAR, 1, 1, ANIM_LOOP };

/* jellyfish bob: 32 steps of a sine, in 12.4 */
static const int8_t BOB[32] = {
    0, 3, 6, 8, 10, 12, 13, 14, 14, 14, 13, 12, 10, 8, 6, 3,
    0, -3, -6, -8, -10, -12, -13, -14, -14, -14, -13, -12, -10, -8, -6, -3,
};

static const shore_t *shore;
static game_entity_t *live[MAX_SPAWNS];
static uint8_t done[MAX_SPAWNS];
static uint8_t squawk_cool;

void actors_reset(void) {
    uint8_t i;
    shore = &SHORES[game.shore];
    for (i = 0; i < MAX_SPAWNS; i++) {
        live[i] = NULL;
        done[i] = 0;
    }
    squawk_cool = 0;
}

void effect_spawn(uint8_t type, int16_t px, int16_t py) {
    game_entity_t *e = physics_entities_spawn(&game_entities, type, px, py, 1, 1);
    if (!e) return;   // effects are optional
    switch (type) {
        case E_SPLASH:
            animated_physics_entity_play(&e->entity, &A_SPLASH);
            e->timer = 12;
            break;
        case E_BUBBLE:
            animated_physics_entity_play(&e->entity, &A_BUBBLE);
            e->timer = 90;
            e->entity.physics.body.vy = -10;
            break;
        case E_STAR:
            animated_physics_entity_play(&e->entity, &A_STAR);
            e->timer = 24;
            e->entity.physics.body.vy = -16;
            break;
    }
}

static game_entity_t *spawn(uint8_t i) {
    const spawn_t *s = &shore_spawns[i];
    int16_t x = (int16_t)s->tx * 16;
    int16_t y = (int16_t)s->ty * 16;
    game_entity_t *e = NULL;
    switch (s->type) {
        case SP_FISH:
            e = physics_entities_spawn(&game_entities, E_FISH, x + 2, y + 5, 12, 6);
            if (!e) break;
            e->entity.physics.visual.ox = -2;
            e->entity.physics.visual.oy = -5;
            animated_physics_entity_play(&e->entity, &A_FISH);
            e->timer = (uint8_t)(i * 37);
            break;
        case SP_CRAB:
            e = physics_entities_spawn(&game_entities, E_CRAB, x + 2, y + 8, 12, 8);
            if (!e) break;
            e->entity.physics.visual.ox = -2;
            e->entity.physics.visual.oy = -8;
            e->entity.physics.body.vx = (i & 1) ? CRAB_SPEED : -CRAB_SPEED;
            animated_physics_entity_play(&e->entity, &A_CRAB);
            break;
        case SP_JELLY:
            e = physics_entities_spawn(&game_entities, E_JELLY, x + 3, y + 3, 10, 10);
            if (!e) break;
            e->entity.physics.visual.ox = -3;
            e->entity.physics.visual.oy = -3;
            e->timer = (uint8_t)(i * 11);
            animated_physics_entity_play(&e->entity, &A_JELLY);
            break;
        case SP_GULL:
            e = physics_entities_spawn(&game_entities, E_GULL, x + 2, y + 4, 12, 8);
            if (!e) break;
            e->entity.physics.visual.ox = -2;
            e->entity.physics.visual.oy = -4;
            e->entity.physics.body.vx = GULL_SPEED;
            animated_physics_entity_play(&e->entity, &A_GULL);
            break;
        case SP_BUOY:
            // stands on the tile below: the 32 px sprite ends on the ground
            e = physics_entities_spawn(&game_entities, E_BUOY, x + 4, y - 12, 8, 28);
            if (!e) break;
            e->entity.physics.visual.tall = 1;
            e->entity.physics.visual.ox = -4;
            e->entity.physics.visual.oy = -4;
            animated_physics_entity_play(&e->entity, done[i] ? &A_BUOY_ON : &A_BUOY);
            break;
        case SP_MOM:
            e = physics_entities_spawn(&game_entities, E_MOM, x + 2, y - 12, 12, 28);
            if (!e) break;
            e->entity.physics.visual.tall = 1;
            e->entity.physics.visual.ox = -2;
            e->entity.physics.visual.oy = -4;
            e->entity.physics.visual.flip = SPRITE_FLIP_X;
            animated_physics_entity_play(&e->entity, &A_MOM);
            break;
    }
    if (e) e->data = i;
    return e;
}

/* spawn what came near the camera, free what drifted away */
static void window(void) {
    uint8_t i;
    int16_t left = cam.x - WINDOW;
    int16_t right = cam.x + SCREEN_W + WINDOW;
    for (i = 0; i < shore->spawn_count; i++) {
        int16_t x = (int16_t)shore_spawns[i].tx * 16;
        uint8_t near = x >= left && x < right;
        if (near && !live[i] && !(done[i] && shore_spawns[i].type <= SP_CRAB)) {
            live[i] = spawn(i);
        } else if (!near && live[i]) {
            entity_pool_free(&game_entities, live[i]);
            live[i] = NULL;
        }
    }
}

static void kill(game_entity_t *e) {
    live[e->data] = NULL;
    entity_pool_free(&game_entities, e);
}

static void crab(game_entity_t *e) {
    body_t *b = &e->entity.physics.body;
    int16_t front;
    b->vy += 6;
    if (b->vy > 96) b->vy = 96;
    body_step(b);
    if (b->state & BODY_HIT_WALL) {
        b->vx = b->vx > 0 ? -CRAB_SPEED : CRAB_SPEED;
    } else if (b->state & BODY_ON_GROUND) {
        // turn before walking off a ledge
        front = b->vx > 0 ? PHYSICS_FP_INT(b->x) + b->w + 1 : PHYSICS_FP_INT(b->x) - 2;
        if (!(tileflags_at(front, PHYSICS_FP_INT(b->y) + b->h + 1) & (TF_SOLID | TF_ONEWAY))) b->vx = -b->vx;
    }
    if (b->vx == 0) b->vx = CRAB_SPEED;
    e->entity.physics.visual.flip = b->vx > 0 ? SPRITE_FLIP_X : 0;
}

static void gull(game_entity_t *e, game_entity_t *pup) {
    body_t *b = &e->entity.physics.body;
    const spawn_t *s = &shore_spawns[e->data];
    int16_t home_x = (int16_t)s->tx * 16, home_y = (int16_t)s->ty * 16 + 4;
    int16_t x = PHYSICS_FP_INT(b->x), y = PHYSICS_FP_INT(b->y);

    if (e->timer) {
        // diving at the pup
        e->timer--;
        if (y > SURFACE_Y - 20 || (b->state & BODY_ON_GROUND) || e->timer == 0) {
            e->timer = 0;
            animated_physics_entity_play(&e->entity, &A_GULL);
        }
    } else if (y > home_y) {
        b->vy = -16;   // climb back
    } else {
        b->vy = 0;
        if (x > home_x + GULL_RANGE) b->vx = -GULL_SPEED;
        else if (x < home_x - GULL_RANGE) b->vx = GULL_SPEED;
        if (pup && pup_alive() && !pup_in_water()) {
            int16_t px = PHYSICS_FP_INT(pup->entity.physics.body.x), py = PHYSICS_FP_INT(pup->entity.physics.body.y);
            int16_t dx = px - x;
            if (dx > -GULL_SIGHT && dx < GULL_SIGHT && py > y + 24) {
                int16_t dy = py - y;
                // aim: cover dx and dy over the same number of frames
                e->timer = GULL_DIVE;
                b->vy = 40;
                // |dx| < GULL_SIGHT (80): dx * 40 fits signed 16 bits.
                b->vx = (dx * 40) / (dy > 16 ? dy : 16);
                if (b->vx > 40) b->vx = 40;
                if (b->vx < -40) b->vx = -40;
                animated_physics_entity_play(&e->entity, &A_GULL_DIVE);
                if (!squawk_cool) {
                    game_sfx_squawk();
                    squawk_cool = 60;
                }
            }
        }
    }
    b->x += b->vx;
    b->y += b->vy;
    e->entity.physics.visual.flip = b->vx < 0 ? SPRITE_FLIP_X : 0;
}

void actors_update(void) {
    game_entity_t *e, *pup = pup_entity();
    uint8_t touch;

    window();
    if (squawk_cool) squawk_cool--;

    ENTITY_POOL_FOR_EACH(&game_entities, e, entity.physics.visual.type) {
        if (e->entity.physics.visual.type == E_PUP) continue;
        touch = pup && pup_alive() && body_overlap(&e->entity.physics.body, &pup->entity.physics.body);
        switch (e->entity.physics.visual.type) {
            case E_FISH:
                e->timer++;
                if (PHYSICS_FP_INT(e->entity.physics.body.y) >= SURFACE_Y) {
                    // swim to and fro around home
                    e->entity.physics.body.x += (e->timer & 64) ? -3 : 3;
                    e->entity.physics.visual.flip = (e->timer & 64) ? SPRITE_FLIP_X : 0;
                } else {
                    e->entity.physics.body.y += BOB[(e->timer >> 1) & 31] >> 2;
                }
                if (touch) {
                    done[e->data] = 1;
                    game.fish++;
                    game.fish_total++;
                    game_sfx_fish();
                    effect_spawn(E_STAR, PHYSICS_FP_INT(e->entity.physics.body.x), PHYSICS_FP_INT(e->entity.physics.body.y) - 6);
                    kill(e);
                    continue;
                }
                break;
            case E_CRAB:
                crab(e);
                if (touch && pup_sliding()) {
                    // a belly slide bowls it over and off the ledge for good
                    done[e->data] = 1;
                    game_sfx_kick();
                    effect_spawn(E_STAR, PHYSICS_FP_INT(e->entity.physics.body.x), PHYSICS_FP_INT(e->entity.physics.body.y) - 6);
                    kill(e);
                    continue;
                }
                if (touch) pup_hurt(PHYSICS_FP_INT(e->entity.physics.body.x) + 6);
                break;
            case E_JELLY:
                e->timer++;
                e->entity.physics.body.y += BOB[(e->timer >> 2) & 31];
                if (touch) pup_hurt(PHYSICS_FP_INT(e->entity.physics.body.x) + 5);
                break;
            case E_GULL:
                gull(e, pup);
                if (touch && !pup_sliding()) pup_hurt(PHYSICS_FP_INT(e->entity.physics.body.x) + 6);   // flat on the ice it can't grab
                break;
            case E_BUOY:
                if (touch && !done[e->data]) {
                    done[e->data] = 1;
                    animated_physics_entity_play(&e->entity, &A_BUOY_ON);
                    pup_set_checkpoint(PHYSICS_FP_INT(e->entity.physics.body.x) - 3, PHYSICS_FP_INT(e->entity.physics.body.y) + 28 - 12);
                    game_sfx_buoy();
                }
                break;
            case E_MOM:
                if (touch && game.state == GS_PLAY) {
                    game.state = GS_CLEAR;
                    game.timer = 0;
                }
                break;
            case E_SPLASH:
            case E_STAR:
                e->entity.physics.body.y += e->entity.physics.body.vy;
                if (--e->timer == 0) {
                    entity_pool_free(&game_entities, e);
                    continue;
                }
                break;
            case E_BUBBLE:
                e->entity.physics.body.y += e->entity.physics.body.vy;
                e->entity.physics.body.x += (e->timer & 16) ? 4 : -4;
                if (--e->timer == 0 || PHYSICS_FP_INT(e->entity.physics.body.y) < SURFACE_Y) {
                    entity_pool_free(&game_entities, e);
                    continue;
                }
                break;
        }
        e->entity.physics.visual.sprite->tile = anim_tick(&e->entity.anim);
    }
}
