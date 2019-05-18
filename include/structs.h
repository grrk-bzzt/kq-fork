/** \page License
 * KQ is Copyright (C) 2002 by Josh Bolduc
 *
 * This file is part of KQ... a freeware RPG.
 *
 * KQ is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2, or (at your
 * option) any later version.
 *
 * KQ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KQ; see the file COPYING.  If not, write to
 * the Free Software Foundation,
 *     675 Mass Ave, Cambridge, MA 02139, USA.
 */

#pragma once

/** \file
 * \brief Structures common to mapedit and the game (s_map and s_entity)
 * \author PH
 * \date 20030805
 */

#include "bounds.h"
#include "enums.h"
#include "fighter.h"
#include "markers.h"
#include "res.h"
class Raster;

enum eHeroBitFlags
{
    BITS_NO_HERO = 0,
    BITS_SENSAR = 1 << 0,
    BITS_SARINA = 1 << 1,
    BITS_CORIN = 1 << 2,
    BITS_AJATHAR = 1 << 3,
    BITS_CASANDRA = 1 << 4,
    BITS_TEMMIN = 1 << 5,
    BITS_AYLA = 1 << 6,
    BITS_NOSLOM = 1 << 7,

    BITS_ALL_HERO = BITS_SENSAR | BITS_SARINA | BITS_CORIN | BITS_AJATHAR | BITS_CASANDRA | BITS_TEMMIN | BITS_AYLA | BITS_NOSLOM
};

constexpr uint8_t MAX_FRAMECTR = 20;
constexpr size_t MAX_SCRIPT = 60;

/** \brief Entity
 *
 * Contains info on an entities appearance, position and behavior.
 */
struct KQEntity
{
    void setIdentity(uint8_t identity) {
        chrx = identity;
    }
    uint8_t identity() const {
        return chrx;
    }
    bool isIdentityAnEnemy() const {
        // 38 looks like some kind of empty cloak with no face.
        return chrx == 38;
    }
    void swapWithIdentity(KQEntity& rhs) {
        std::swap(eid, rhs.eid);
        std::swap(chrx, rhs.chrx);
        std::swap(active, rhs.active);
    }

    // Entity's identity (what s/he looks like)
    uint8_t chrx;

    // x-coord on map
    uint16_t x;

    // y-coord on map
    uint16_t y;

    // x-coord tile that entity is standing on
    uint16_t tilex;

    // y-coord tile that entity is standing on
    uint16_t tiley;

    // Entity type (fighter, enemy, normal)
    uint8_t eid;

    // "Alive" or not
    bool active;

    // Direction
    uint8_t facing;

    // In the middle of a move
    bool moving;

    // How far along the move entity is
    uint8_t movcnt;

    // Counter for determining animation frame: [0..MAX_FRAMECTR)
    uint8_t framectr;

    // Stand, wander, script or chasing
    uint8_t movemode;

    // Determine if affected by obstacles or not
    uint8_t obsmode;

    // Movement delay (between steps)
    uint8_t delay;

    // Counter for movement delay
    uint8_t delayctr;

    // How hyperactive the entity is
    uint8_t speed;

    uint8_t scount;

    // Scripted commands (eCommands in entity.h)
    uint8_t cmd;

    // Index within script parser
    uint8_t sidx;

    uint8_t extra;

    // Entity is following another
    uint8_t chasing;

    // Number of times we need to repeat 'cmd'
    signed int cmdnum;

    uint8_t atype;

    // Snaps back to direction previously facing
    uint8_t snapback;

    // Look at player when talked to
    uint8_t facehero;

    // Entity is see-through or not
    bool isSemiTransparent;

    // Movement/action script (pacing, etc.)
    char script[MAX_SCRIPT];

    // Scripted x-coord the ent is moving to
    uint16_t target_x;

    // Scripted y-coord the ent is moving to
    uint16_t target_y;
};

/** \brief Animation specifier
 *
 * Marks a block of tiles that are interchanged to give
 * an animation effect. Used in check_animation()
 */
struct s_anim
{
    // First tile in sequence
    uint16_t start;

    // Last tile in sequence
    uint16_t end;

    // Frames to wait between tile changes
    uint16_t delay;
};

/** \brief Tileset definition
 *
 * Encapsulates a tileset's graphics and animations.
 */
struct s_tileset
{
    char icon_set[16];
    s_anim tanim[MAX_ANIM];
};

/** \brief Progress Dump
 *
 * Contains the names of all the P_* progress constants
 */
struct s_progress
{
    // Number of current progress
    uint32_t num_progress;

    // Name of current progress
    char progressName[18];
};

/*! \brief Hero information
 *
 * This holds static or constant information about a hero.
 * the intention is to cut down on some of those globals.
 */
struct s_heroinfo
{
    // The hero's portrait for the stats screen
    Raster* portrait;

    // Frames for movement
    Raster* frames[MAX_PARTY_MOVEMENT_FRAMES];

    // Frames for combat
    Raster* cframes[MAXCFRAMES];
};

/** \brief Special Items
 *
 * Contains a list of the special items in the player's party (Opal Armor et al)
 */
struct s_special_item
{
    char specialItemName[38];
    char description[40];
    short icon;
};

/** \brief Inventory
* An item ID and the quantity of that thing in the inventory.
*/
struct s_inventory
{
    unsigned short item;
    unsigned short quantity;
};

/** \brief Save Game Stats
 * The information that's shown when picking a slot to save/load.
 */
struct s_sgstats
{
    int num_characters;
    int gold;
    int time;
    struct
    {
        int id;
        int level;
        int hp;
        int mp;
    } characters[MAX_PARTY_SIZE];
    static s_sgstats get_current();
};
