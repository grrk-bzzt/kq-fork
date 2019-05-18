#pragma once

/*! \file
 * \brief Stuff to work with magic.
 *
 * This includes the M_* defines
 * and some function prototypes.
 */

#include "kq.h"
#include "structs.h"

enum EMagic
{
    M_CURE1 = 1,
    M_HOLD = 2,
    M_SHIELD = 3,
    M_SILENCE = 4,
    M_SLEEP = 5,
    M_BLESS = 6,
    M_VISION = 7,
    M_CURE2 = 8,
    M_HOLYMIGHT = 9,
    M_RESTORE = 10,
    M_FADE = 11,
    M_HASTEN = 12,
    M_LIFE = 13,
    M_SHELL = 14,
    M_WHIRLWIND = 15,
    M_FLOOD = 16,
    M_RECOVERY = 17,
    M_SHIELDALL = 18,
    M_SLEEPALL = 19,
    M_CURE3 = 20,
    M_REGENERATE = 21,
    M_REPULSE = 22,
    M_THROUGH = 23,
    M_QUICKEN = 24,
    M_TRUEAIM = 25,
    M_WALL = 26,
    M_DIVINEGUARD = 27,
    M_TORNADO = 28,
    M_FULLLIFE = 29,
    M_CURE4 = 30,
    M_LUMINE = 31,
    M_TSUNAMI = 32,
    M_VENOM = 33,
    M_SCORCH = 34,
    M_BLIND = 35,
    M_CONFUSE = 36,
    M_SHOCK = 37,
    M_GLOOM = 38,
    M_NAUSEA = 39,
    M_FROST = 40,
    M_SLOW = 41,
    M_DRAIN = 42,
    M_FIREBLAST = 43,
    M_WARP = 44,
    M_STONE = 45,
    M_LIGHTNING = 46,
    M_VIRUS = 47,
    M_TREMOR = 48,
    M_ABSORB = 49,
    M_DIFFUSE = 50,
    M_DOOM = 51,
    M_MALISON = 52,
    M_FLAMEWALL = 53,
    M_BLIZZARD = 54,
    M_DEATH = 55,
    M_THUNDERSTORM = 56,
    M_NEGATIS = 57,
    M_EARTHQUAKE = 58,
    M_PLAGUE = 59,
    M_XSURGE = 60,
};

/*! non-combat spell states */
#define P_REPULSE 48

int combat_spell(size_t, int);
int cast_spell(size_t, int);
void cast_imbued_spell(size_t, int, int, int);
void special_damage_oneall_enemies(size_t, int, int, size_t,
                                   int);
int res_adjust(size_t, size_t, int);
int res_throw(int, int);
int non_dmg_save(int, int);
int mp_needed(size_t, int);
void adjust_hp(size_t,
               int);
void adjust_mp(size_t, int);
int do_shell_check(int, int);
int do_shield_check(int, int);
KFighter status_adjust(size_t fighter_index);
