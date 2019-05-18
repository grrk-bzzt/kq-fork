/**
 * Entity functions
 */

#include "combat.h"
#include "entity.h"
#include "enums.h"
#include "input.h"
#include "intrface.h"
#include "itemdefs.h"
#include "kq.h"
#include "menu.h"
#include "random.h"
#include "setup.h"

KEntity kEntity;

KEntity::KEntity()
{
}

KEntity::~KEntity()
{
}

void KEntity::ChaseAfterMainPlayer(t_entity target_entity)
{
    if (target_entity >= MAX_ENTITIES)
    {
        return;
    }

    auto& entity = g_ent[target_entity];
    if (entity.chasing == 0)
    {
        if (entity_near(target_entity, 0, 3) == 1 && kqrandom->random_range_exclusive(0, 100) <= entity.extra)
        {
            entity.chasing = 1;
            if (entity.speed < 7)
            {
                entity.speed++;
            }
            entity.delay = 0;
        }
        else
        {
            wander(target_entity);
        }
    }
    if (entity.chasing == 1)
    {
        if (entity_near(target_entity, 0, 4) == 1)
        {
            bool emoved = false;

            if (g_ent[0].tilex > entity.tilex)
            {
                emoved = move(target_entity, 1, 0);
            }
            if (!emoved && g_ent[0].tilex < entity.tilex)
            {
                emoved = move(target_entity, -1, 0);
            }
            if (!emoved && g_ent[0].tiley > entity.tiley)
            {
                emoved = move(target_entity, 0, 1);
            }
            if (!emoved && g_ent[0].tiley < entity.tiley)
            {
                emoved = move(target_entity, 0, -1);
            }
            if (!emoved)
            {
                wander(target_entity);
            }
        }
        else
        {
            entity.chasing = 0;
            if (entity.speed > 1)
            {
                entity.speed--;
            }
            entity.delay = kqrandom->random_range_exclusive(25, 50);
            wander(target_entity);
        }
    }
}

void KEntity::recalculateNumberOfActiveMapEntities()
{
    noe = 0;
    for (size_t entity_index = 0; entity_index < MAX_ENTITIES; entity_index++)
    {
        if (g_ent[entity_index].active)
        {
            noe = entity_index + 1;
        }
    }
}

void KEntity::setNumberOfMapEntities(size_t numEntities)
{
    noe = numEntities;
}

size_t KEntity::getNumberOfMapEntities() const
{
    return noe;
}

int KEntity::entity_near(t_entity eno, t_entity tgt, int rad)
{
    int ax, ay, ex, ey, b;

    b = 0 - rad;
    ex = g_ent[eno].tilex;
    ey = g_ent[eno].tiley;
    for (ay = b; ay <= rad; ay++)
    {
        for (ax = b; ax <= rad; ax++)
        {
            if (ex + ax >= view_x1 && ax + ax <= view_x2 && ey + ay >= view_y1 && ey + ay <= view_y2)
            {
                if (ex + ax == g_ent[tgt].tilex && ey + ay == g_ent[tgt].tiley)
                {
                    return 1;
                }
            }
        }
    }
    return 0;
}

int KEntity::entityat(int ox, int oy, t_entity who)
{
    for (t_entity i = 0; i < MAX_ENTITIES; i++)
    {
        if (g_ent[i].active && ox == g_ent[i].tilex && oy == g_ent[i].tiley)
        {
            if (who >= MAX_PARTY_SIZE)
            {
                if (g_ent[who].eid == ID_ENEMY && i < MAX_PARTY_SIZE)
                {
                    if (kqCombat.combat(0) == 1)
                    {
                        g_ent[who].active = false;
                    }
                    return 0;
                }
                return i + 1;
            }
            else
            {
                if (g_ent[i].eid == ID_ENEMY)
                {
                    if (kqCombat.combat(0) == 1)
                    {
                        g_ent[i].active = false;
                    }
                    return 0;
                }
                if (i >= MAX_PARTY_SIZE)
                {
                    return i + 1;
                }
            }
        }
    }
    return 0;
}

void KEntity::entscript(t_entity target_entity)
{
    if (target_entity >= MAX_ENTITIES)
    {
        return;
    }

    auto& entity = g_ent[target_entity];
    if (!entity.active)
    {
        return;
    }
    if (entity.cmd == 0)
    {
        getcommand(target_entity);
    }
    switch (entity.cmd)
    {
    case eCommands::COMMAND_MOVE_UP:
        if (move(target_entity, 0, -1))
        {
            entity.cmdnum--;
        }
        break;
    case eCommands::COMMAND_MOVE_DOWN:
        if (move(target_entity, 0, 1))
        {
            entity.cmdnum--;
        }
        break;
    case eCommands::COMMAND_MOVE_LEFT:
        if (move(target_entity, -1, 0))
        {
            entity.cmdnum--;
        }
        break;
    case eCommands::COMMAND_MOVE_RIGHT:
        if (move(target_entity, 1, 0))
        {
            entity.cmdnum--;
        }
        break;
    case eCommands::COMMAND_WAIT:
        entity.cmdnum--;
        break;
    case eCommands::COMMAND_FINISH_COMMANDS:
        return;
    case eCommands::COMMAND_REPEAT:
        entity.sidx = 0;
        entity.cmdnum = 0;
        break;
    case eCommands::COMMAND_MOVETO_X:
        if (entity.tilex < entity.cmdnum)
        {
            move(target_entity, 1, 0);
        }
        if (entity.tilex > entity.cmdnum)
        {
            move(target_entity, -1, 0);
        }
        if (entity.tilex == entity.cmdnum)
        {
            entity.cmdnum = 0;
        }
        break;
    case eCommands::COMMAND_MOVETO_Y:
        if (entity.tiley < entity.cmdnum)
        {
            move(target_entity, 0, 1);
        }
        if (entity.tiley > entity.cmdnum)
        {
            move(target_entity, 0, -1);
        }
        if (entity.tiley == entity.cmdnum)
        {
            entity.cmdnum = 0;
        }
        break;
    case eCommands::COMMAND_FACE:
        entity.facing = entity.cmdnum;
        entity.cmdnum = 0;
        break;
    }
    if (entity.cmdnum == 0)
    {
        entity.cmd = 0;
    }
}

void KEntity::follow(int tile_x, int tile_y)
{
    t_entity i;

    if (numchrs == 1)
    {
        return;
    }
    for (i = numchrs - 1; i > 0; --i)
    {
        if (i == 1)
        {
            move(i, tile_x - g_ent[i].tilex, tile_y - g_ent[i].tiley);
        }
        else
        {
            move(i, g_ent[i - 1].tilex - g_ent[i].tilex, g_ent[i - 1].tiley - g_ent[i].tiley);
        }
    }
}

void KEntity::getcommand(t_entity target_entity)
{
    if (target_entity >= MAX_ENTITIES)
    {
        return;
    }

    auto& entity = g_ent[target_entity];

    /* PH FIXME: prevented from running off end of string */
    char currentCommand = '\0';
    const auto scriptIndex = entity.sidx;
    if (scriptIndex < MAX_SCRIPT)
    {
        currentCommand = entity.script[scriptIndex];
        entity.sidx++;
    }

    switch (currentCommand)
    {
    case 'u':
    case 'U':
        entity.cmd = eCommands::COMMAND_MOVE_UP;
        parsems(target_entity);
        break;
    case 'd':
    case 'D':
        entity.cmd = eCommands::COMMAND_MOVE_DOWN;
        parsems(target_entity);
        break;
    case 'l':
    case 'L':
        entity.cmd = eCommands::COMMAND_MOVE_LEFT;
        parsems(target_entity);
        break;
    case 'r':
    case 'R':
        entity.cmd = eCommands::COMMAND_MOVE_RIGHT;
        parsems(target_entity);
        break;
    case 'w':
    case 'W':
        entity.cmd = eCommands::COMMAND_WAIT;
        parsems(target_entity);
        break;
    case '\0':
        entity.cmd = eCommands::COMMAND_FINISH_COMMANDS;
        entity.movemode = MM_STAND;
        entity.cmdnum = 0;
        entity.sidx = 0;
        break;
    case 'b':
    case 'B':
        entity.cmd = eCommands::COMMAND_REPEAT;
        break;
    case 'x':
    case 'X':
        entity.cmd = eCommands::COMMAND_MOVETO_X;
        parsems(target_entity);
        break;
    case 'y':
    case 'Y':
        entity.cmd = eCommands::COMMAND_MOVETO_Y;
        parsems(target_entity);
        break;
    case 'f':
    case 'F':
        entity.cmd = eCommands::COMMAND_FACE;
        parsems(target_entity);
        break;
    case 'k':
    case 'K':
        /* PH add: command K makes the ent disappear */
        entity.cmd = eCommands::COMMAND_KILL;
        entity.active = false;
        break;
    default:
#ifdef DEBUGMODE
        if (debugging > 0)
        {
            sprintf(strbuf, _("Invalid entity command (%c) at position %d for ent %d"), currentCommand, entity.sidx, target_entity);
            Game.program_death(strbuf);
        }
#endif
        break;
    }
}

bool KEntity::move(t_entity target_entity, int dx, int dy)
{
    if (target_entity >= MAX_ENTITIES)
    {
        return false;
    }

    auto& entity = g_ent[target_entity];

    if (dx == 0 && dy == 0) // Speed optimization.
    {
        return false;
    }

    int tile_x = entity.x / TILE_W;
    int tile_y = entity.y / TILE_H;
    int oldfacing = entity.facing;
    if (dx < 0)
    {
        entity.facing = FACE_LEFT;
    }
    else if (dx > 0)
    {
        entity.facing = FACE_RIGHT;
    }
    else if (dy > 0)
    {
        entity.facing = FACE_DOWN;
    }
    else if (dy < 0)
    {
        entity.facing = FACE_UP;
    }
    if (tile_x + dx < 0 || tile_x + dx >= (int)g_map.xsize ||
        tile_y + dy < 0 || tile_y + dy >= (int)g_map.ysize)
    {
        return false;
    }
    if (entity.obsmode == 1)
    {
        // Try to automatically walk/run around obstacle.
        if (dx && obstruction(tile_x, tile_y, dx, 0, FALSE))
        {
            if (dy != -1 && oldfacing == entity.facing &&
                !obstruction(tile_x, tile_y + 1, dx, 0, TRUE) &&
                !obstruction(tile_x, tile_y, 0, 1, TRUE))
            {
                dy = 1;
            }
            else if (dy != 1 && oldfacing == entity.facing &&
                !obstruction(tile_x, tile_y - 1, dx, 0, TRUE) &&
                !obstruction(tile_x, tile_y, 0, -1, TRUE))
            {
                dy = -1;
            }
            else
            {
                dx = 0;
            }
        }
        if (dy && obstruction(tile_x, tile_y, 0, dy, FALSE))
        {
            if (dx != -1 && oldfacing == entity.facing &&
                !obstruction(tile_x + 1, tile_y, 0, dy, TRUE) &&
                !obstruction(tile_x, tile_y, 1, 0, TRUE))
            {
                dx = 1;
            }
            else if (dx != 1 && oldfacing == entity.facing &&
                !obstruction(tile_x - 1, tile_y, 0, dy, TRUE) &&
                !obstruction(tile_x, tile_y, -1, 0, TRUE))
            {
                dx = -1;
            }
            else
            {
                dy = 0;
            }
        }
        if ((dx || dy) && obstruction(tile_x, tile_y, dx, dy, FALSE))
        {
            dx = dy = 0;
        }
    }

    if (!dx && !dy && oldfacing == entity.facing)
    {
        return false;
    }

    if (entity.obsmode == 1 && entityat(tile_x + dx, tile_y + dy, target_entity))
    {
        return false;
    }

    // Make sure that the player can't avoid special zones by moving diagonally.
    if (dx && dy)
    {
        int source_tile = tile_y * g_map.xsize + tile_x;
        if (z_seg[source_tile] != z_seg[source_tile + dx] ||
            z_seg[source_tile] != z_seg[source_tile + dy * g_map.xsize])
        {
            if (entity.facing == FACE_LEFT || entity.facing == FACE_RIGHT)
            {
                if (!obstruction(tile_x, tile_y, dx, 0, TRUE))
                {
                    dy = 0;
                }
                else
                {
                    dx = 0;
                }
            }
            else   // They are facing up or down.
            {
                if (!obstruction(tile_x, tile_y, 0, dy, TRUE))
                {
                    dx = 0;
                }
                else
                {
                    dy = 0;
                }
            }
        }
    }

    // Make sure player can't walk diagonally between active entities.
    if (dx && dy)
    {
        if (obstruction(tile_x, tile_y, dx, 0, TRUE) &&
            obstruction(tile_x, tile_y, 0, dy, TRUE))
        {
            return false;
        }
    }

    entity.tilex = tile_x + dx;
    entity.tiley = tile_y + dy;
    entity.y += dy;
    entity.x += dx;
    entity.moving = true;
    entity.movcnt = 15;
    return true;
}

int KEntity::obstruction(int origin_x, int origin_y, int move_x, int move_y, int check_entity)
{
    int current_tile; // obstrution for current tile
    int target_tile;  // obstruction for destination tile
    int dest_x;       // destination tile, x-coord
    int dest_y;       // destination tile, y-coord
    t_entity i;

    // Block entity if it tries to walk off the map
    if ((origin_x == 0 && move_x < 0) ||
        (origin_y == 0 && move_y < 0) ||
        (origin_x == (int)g_map.xsize - 1 && move_x > 0) ||
        (origin_y == (int)g_map.ysize - 1 && move_y > 0))
    {
        return 1;
    }

    dest_x = origin_x + move_x;
    dest_y = origin_y + move_y;

    // Check the current and target tiles' obstacles
    current_tile = o_seg[(origin_y * g_map.xsize) + origin_x];
    target_tile = o_seg[(dest_y * g_map.xsize) + dest_x];

    // Return early if the destination tile is an obstruction
    if (target_tile == BLOCK_ALL)
    {
        return 1;
    }

    // Check whether the current OR target tiles block movement
    if (move_y == -1)
    {
        if (current_tile == BLOCK_U || target_tile == BLOCK_D)
        {
            return 1;
        }
    }
    if (move_y == 1)
    {
        if (current_tile == BLOCK_D || target_tile == BLOCK_U)
        {
            return 1;
        }
    }
    if (move_x == -1)
    {
        if (current_tile == BLOCK_L || target_tile == BLOCK_R)
        {
            return 1;
        }
    }
    if (move_x == 1)
    {
        if (current_tile == BLOCK_R || target_tile == BLOCK_L)
        {
            return 1;
        }
    }

    // Another entity blocks movement as well
    if (check_entity)
    {
        for (i = 0; i < MAX_ENTITIES; i++)
        {
            if (g_ent[i].active && dest_x == g_ent[i].tilex && dest_y == g_ent[i].tiley)
            {
                return 1;
            }
        }
    }

    // No obstacles found
    return 0;
}

void KEntity::parsems(t_entity target_entity)
{
    if (target_entity >= MAX_ENTITIES)
    {
        return;
    }

    auto& entity = g_ent[target_entity];

    size_t scriptIndex = entity.sidx;
    if (scriptIndex >= MAX_SCRIPT)
    {
        return;
    }

    // 48..57 are '0'..'9' ASCII
    char s = entity.script[scriptIndex];
    uint32_t p = 0;
    char tok[10];
    while (s >= '0' && s <= '9' && p < sizeof(tok) - 1)
    {
        tok[p] = s;
        p++;

        entity.sidx++;
        scriptIndex = entity.sidx;
        if (scriptIndex >= MAX_SCRIPT)
        {
            break;
        }
        s = entity.script[scriptIndex];
    }
    tok[p] = 0;
    entity.cmdnum = atoi(tok);
}

void KEntity::place_ent(t_entity entity_index, int ex, int ey)
{
    g_ent[entity_index].tilex = ex;
    g_ent[entity_index].tiley = ey;
    g_ent[entity_index].x = g_ent[entity_index].tilex * TILE_W;
    g_ent[entity_index].y = g_ent[entity_index].tiley * TILE_H;
}

void KEntity::player_move()
{
    int oldx = g_ent[0].tilex;
    int oldy = g_ent[0].tiley;

    PlayerInput.readcontrols();

    if (PlayerInput.balt)
    {
        Game.activate();
    }
    if (PlayerInput.benter)
    {
        menu();
    }
#ifdef KQ_CHEATS
    if (PlayerInput.bcheat)
    {
        do_luacheat();
    }
#endif

    move(0,
         PlayerInput.right ? 1 : PlayerInput.left ? -1 : 0,
         PlayerInput.down  ? 1 : PlayerInput.up   ? -1 : 0);
    if (g_ent[0].moving)
    {
        follow(oldx, oldy);
    }
}

void KEntity::process_entities()
{
    for (t_entity i = 0; i < MAX_ENTITIES; i++)
    {
        if (g_ent[i].active)
        {
            speed_adjust(i);
        }
    }

    /* Do timers */
    char* t_evt = Game.get_timer_event();
    if (t_evt)
    {
        do_timefunc(t_evt);
    }
}

void KEntity::process_entity(t_entity target_entity)
{
    auto& entity = g_ent[target_entity];
    KPlayer* player = nullptr;

    entity.scount = 0;

    if (!entity.active)
    {
        return;
    }

    if (!entity.moving)
    {
        if (target_entity == 0 && !autoparty)
        {
            player_move();
            if (entity.moving && display_desc == 1)
            {
                display_desc = 0;
            }
            return;
        }
        switch (entity.movemode)
        {
        case MM_STAND:
            return;
        case MM_WANDER:
            wander(target_entity);
            break;
        case MM_SCRIPT:
            entscript(target_entity);
            break;
        case MM_CHASE:
            ChaseAfterMainPlayer(target_entity);
            break;
        case MM_TARGET:
            target(target_entity);
            break;
        }
    }
    else   /* if (.moving==0) */
    {
        if (entity.tilex * TILE_W > entity.x)
        {
            ++entity.x;
        }
        if (entity.tilex * TILE_W < entity.x)
        {
            --entity.x;
        }
        if (entity.tiley * TILE_H > entity.y)
        {
            ++entity.y;
        }
        if (entity.tiley * TILE_H < entity.y)
        {
            --entity.y;
        }
        entity.movcnt--;

        if (entity.framectr < MAX_FRAMECTR - 1)
        {
            entity.framectr++;
        }
        else
        {
            entity.framectr = 0;
        }

        if (entity.movcnt == 0)
        {
            entity.moving = false;
            if (target_entity < MAX_PARTY_SIZE)
            {
                player = &party[pidx[target_entity]];
                if (steps < STEPS_NEEDED)
                {
                    steps++;
                }
                if (player->sts[S_POISON] > 0)
                {
                    if (player->hp > 1)
                    {
                        player->hp--;
                    }
                    play_effect(21, 128);
                }
                if (player->eqp[EQP_SPECIAL] == I_REGENERATOR)
                {
                    if (player->hp < player->mhp)
                    {
                        player->hp++;
                    }
                }
            }
            if (target_entity == 0)
            {
                Game.zone_check();
            }
        }

        if (target_entity == 0 && vfollow == 1)
        {
            Game.calc_viewport();
        }
    }
}

void KEntity::set_script(t_entity target_entity, const std::string& movestring)
{
    if (target_entity >= MAX_ENTITIES)
    {
        return;
    }
    auto& entity = g_ent[target_entity];

    entity.moving = false; // Stop entity from moving
    entity.movcnt = 0; // Reset the move counter to 0
    entity.cmd = eCommands::COMMAND_NONE;
    entity.sidx = 0;   // Reset script command index
    entity.cmdnum = 0; // There are no scripted commands
    entity.movemode = MM_SCRIPT; // Force the entity to follow the script
    strncpy(entity.script, movestring.c_str(), sizeof(entity.script));
}

void KEntity::speed_adjust(t_entity target_entity)
{
    auto& entity = g_ent[target_entity];

    if (entity.speed < 4)
    {
        switch (entity.speed)
        {
        case 1:
            if (entity.scount < 3)
            {
                entity.scount++;
                return;
            }
            break;
        case 2:
            if (entity.scount < 2)
            {
                entity.scount++;
                return;
            }
            break;
        case 3:
            if (entity.scount < 1)
            {
                entity.scount++;
                return;
            }
            break;
        }
    }
    if (entity.speed < 5)
    {
        process_entity(target_entity);
    }
    switch (entity.speed)
    {
    case 5:
        process_entity(target_entity);
        process_entity(target_entity);
        break;
    case 6:
        process_entity(target_entity);
        process_entity(target_entity);
        process_entity(target_entity);
        break;
    case 7:
        process_entity(target_entity);
        process_entity(target_entity);
        process_entity(target_entity);
        process_entity(target_entity);
        break;
    }
    /* TT: This is to see if the player is "running" */
    if (key[PlayerInput.kctrl] && target_entity < MAX_PARTY_SIZE)
    {
        process_entity(target_entity);
    }
}

void KEntity::target(t_entity target_entity)
{
    int dx, dy, ax, ay, emoved = 0;
    auto& entity = g_ent[target_entity];

    ax = dx = entity.target_x - entity.tilex;
    ay = dy = entity.target_y - entity.tiley;
    if (ax < 0)
    {
        ax = -ax;
    }
    if (ay < 0)
    {
        ay = -ay;
    }
    if (ax < ay)
    {
        /* Try to move horizontally */
        if (dx < 0)
        {
            emoved = move(target_entity, -1, 0);
        }
        if (dx > 0)
        {
            emoved = move(target_entity, 1, 0);
        }
        /* Didn't move so try vertically */
        if (!emoved)
        {
            if (dy < 0)
            {
                move(target_entity, 0, -1);
            }
            if (dy > 0)
            {
                move(target_entity, 0, 1);
            }
        }
    }
    else
    {
        /* Try to move vertically */
        if (dy < 0)
        {
            emoved = move(target_entity, 0, -1);
        }
        if (dy > 0)
        {
            emoved = move(target_entity, 0, 1);
        }
        /* Didn't move so try horizontally */
        if (!emoved)
        {
            if (dx < 0)
            {
                move(target_entity, -1, 0);
            }
            if (dx > 0)
            {
                move(target_entity, 1, 0);
            }
        }
    }
    if (dx == 0 && dy == 0)
    {
        /* Got there */
        entity.movemode = MM_STAND;
    }
}

void KEntity::wander(t_entity target_entity)
{
    auto& entity = g_ent[target_entity];

    if (entity.delayctr < entity.delay)
    {
        entity.delayctr++;
        return;
    }
    entity.delayctr = 0;
    switch (kqrandom->random_range_exclusive(0, 8))
    {
    case 0:
        move(target_entity, 0, -1);
        break;
    case 1:
        move(target_entity, 0, 1);
        break;
    case 2:
        move(target_entity, -1, 0);
        break;
    case 3:
        move(target_entity, 1, 0);
        break;
    }
}
