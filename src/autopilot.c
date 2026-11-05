/*
 * AUTOPILOT builds only (-DAUTOPILOT=ON): a bot that plays the real game, used to record
 * gameplay videos in the emulator. It returns the buttons a player would hold this frame;
 * everything else (physics, rules, sound) is the normal game.
 *
 * It follows a hand-made route per shore (TMX Routes object layers in assets/shores/): on land it hops toward the next
 * point and jumps at walls, in water it swims straight at it with a kick now and then. A leap
 * point makes it swim straight up with a kick to jump out of the water.
 */
#include "game.h"
#include "route.h"

#define PUP_W 14
#define PUP_H 12

static const waypoint_t *wp;
int16_t ap_dbg[6];   // x, y, vx, vy, waypoint index, leaping (read by tools/trace.py)
static uint8_t leaping;
static uint8_t held_jump;
static uint8_t kick_wait;
static uint8_t stuck;
static uint8_t breathing;
static int16_t last_x;

void autopilot_reset(void) {
    wp = game.shore < ROUTE_COUNT ? ROUTES[game.shore] : NULL;
    leaping = 0;
    held_jump = 0;
    kick_wait = 0;
    stuck = 0;
    breathing = 0;
    last_x = 0;
}

/* back at a buoy: pick the route up again from the first point ahead of the pup */
void autopilot_respawn(void) {
    game_entity_t *pup = pup_entity();
    int16_t x;
    autopilot_reset();
    if (wp == NULL || pup == NULL) return;
    x = PHYSICS_FP_INT(pup->entity.physics.body.x);
    while (wp->flags != R_END && (int16_t)wp->tx * 16 + 8 < x) wp++;
}

static uint16_t press_jump(uint16_t in) {
    // a jump or kick starts on a new press only
    if (held_jump) return in;
    return in | IN_JUMP;
}

/* a crab ahead on this ledge, or a gull coming down: get flat */
static uint8_t slide_now(const body_t *b, int16_t dx) {
    game_entity_t *e;
    int16_t x = PHYSICS_FP_INT(b->x), y = PHYSICS_FP_INT(b->y);
    ENTITY_POOL_FOR_EACH(&game_entities, e, entity.physics.visual.type) {
        int16_t ex, ey;
        ex = PHYSICS_FP_INT(e->entity.physics.body.x) - x;
        ey = PHYSICS_FP_INT(e->entity.physics.body.y) - y;
        if (e->entity.physics.visual.type == E_GULL && e->timer && ex > -64 && ex < 64) return 1;
        if (e->entity.physics.visual.type != E_CRAB || ey < -16 || ey > 16) continue;
        if (dx > 0 ? (ex > -8 && ex < 56) : (ex < 8 && ex > -56)) return 1;
    }
    return 0;
}

/* a jellyfish close by: which way to dodge it (IN_UP / IN_DOWN), or 0 */
static uint16_t jelly_dodge(int16_t cx, int16_t cy) {
    game_entity_t *e;
    ENTITY_POOL_FOR_EACH(&game_entities, e, entity.physics.visual.type) {
        int16_t ex, ey;
        if (e->entity.physics.visual.type != E_JELLY) continue;
        ex = PHYSICS_FP_INT(e->entity.physics.body.x) + 5 - cx;
        ey = PHYSICS_FP_INT(e->entity.physics.body.y) + 5 - cy;
        if (ex < -36 || ex > 36 || ey < -30 || ey > 30) continue;
        return ey > 0 ? IN_UP : IN_DOWN;
    }
    return 0;
}

static uint16_t decide(void) {
    game_entity_t *pup = pup_entity();
    body_t *b;
    int16_t cx, cy, tx, ty, dx, dy, front;
    uint16_t in = 0, dodge;

    if (pup == NULL || !pup_alive() || wp == NULL || wp->flags == R_END) return 0;
    b = &pup->entity.physics.body;
    cx = PHYSICS_FP_INT(b->x) + PUP_W / 2;
    cy = PHYSICS_FP_INT(b->y) + PUP_H / 2;
    tx = (int16_t)wp->tx * 16 + 8;
    ty = (int16_t)wp->ty * 16 + 8;
    dx = tx - cx;
    dy = ty - cy;

    if (leaping) {
        if (!pup_in_water()) {
            // airborne: steer onto the next point
            leaping = 0;
            wp++;
            return 0;
        }
        // straight up; kick once on the way
        in = IN_UP;
        if (b->vy < -16 && kick_wait == 0 && !held_jump) {
            kick_wait = 60;
            return in | IN_JUMP;
        }
        if (kick_wait) kick_wait--;
        return in;
    }

    if (dx > -8 && dx < 8 && dy > -12 && dy < 12) {
        if (wp->flags & R_LEAP) {
            leaping = 1;
            kick_wait = 0;
            return IN_UP;
        }
        wp++;
        return 0;
    }

    if (dx > 4) in |= IN_RIGHT;
    else if (dx < -4) in |= IN_LEFT;

    if (pup_in_water()) {
        // low on air with open water above: go up for a breath first
        if (game.air < AIR_MAX / 4 && !(tileflags_at(cx, SURFACE_Y + 4) & TF_SOLID)) breathing = 1;
        if (breathing) {
            if (game.air == AIR_MAX) breathing = 0;
            return IN_UP;
        }
        if (kick_wait) kick_wait--;
        dodge = jelly_dodge(cx, cy);
        if (dodge) return in | dodge;
        if (dy > 4) in |= IN_DOWN;
        else if (dy < -4) in |= IN_UP;
        if ((dx > 72 || dx < -72 || dy > 72 || dy < -72) && kick_wait == 0 && !held_jump) {
            kick_wait = 30;
            return in | IN_JUMP;
        }
        return in;
    }

    if (wp->flags & R_SLIDE) in |= IN_SLIDE;
    if ((b->state & BODY_ON_GROUND) && slide_now(b, dx)) return in | IN_SLIDE;
    if (b->state & BODY_ON_GROUND) {
        front = dx > 0 ? PHYSICS_FP_INT(b->x) + PUP_W + 2 : PHYSICS_FP_INT(b->x) - 3;
        if ((wp->flags & R_JUMP) || (tileflags_at(front, PHYSICS_FP_INT(b->y) + PUP_H - 4) & TF_SOLID) ||
            (dy < -12 && dx < 24 && dx > -24)) {
            return press_jump(in);
        }
        // not getting anywhere: hop
        if (PHYSICS_FP_INT(b->x) == last_x && (in & (IN_LEFT | IN_RIGHT))) {
            if (++stuck > 20) {
                stuck = 0;
                return press_jump(in);
            }
        } else {
            stuck = 0;
        }
        last_x = PHYSICS_FP_INT(b->x);
    } else if (b->vy < 0) {
        in |= IN_JUMP;   // hold for full height
    }
    return in;
}

uint16_t autopilot_input(void) {
    uint16_t in = decide();
    game_entity_t *pup = pup_entity();
    if (pup) {
        ap_dbg[0] = PHYSICS_FP_INT(pup->entity.physics.body.x);
        ap_dbg[1] = PHYSICS_FP_INT(pup->entity.physics.body.y);
        ap_dbg[2] = pup->entity.physics.body.vx;
        ap_dbg[3] = pup->entity.physics.body.vy;
    }
    ap_dbg[4] = wp ? (int16_t)(wp - ROUTES[game.shore]) : -1;
    ap_dbg[5] = leaping | (pup_in_water() << 4);
    held_jump = (in & IN_JUMP) != 0;
    return in;
}
