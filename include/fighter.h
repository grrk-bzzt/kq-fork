#pragma once

#include <string>

#include <allegro.h>

#include "enums.h"

class Raster;

/** Fighter
 *
 * s_player is transformed into a KFighter during combat.
 * See enemy_init() for more information on the fields.
 */
class KFighter
{
public:
    static constexpr uint32_t kMaxCombatSkills = 8;

    /** Combat skills: 1..99 seem to be player-owned; 100..253 appear to be enemy. 0, 254, 255: not specified.
     * 1..60 are found under the EMagic enum.
     * 101: "Venomous Bite"
     * 102: "Double Slash"
     * 103: "Chill Touch"
     * 104: "Flash Flood"
     * 105: "Sweep"
     * 106: "ParaClaw"
     * 107: "Dragon Bite"
     * 108: "Stone Gas"
     * 109: "Zemmel Rod"
     * 110: "Poison Gas"
     * 111: "Tangle Root"
     * 112: "Petrifying Bite"
     * 113: "Maul of the Titans"
     * 114: "Stunning Strike"
     * 115: "Howl"
     * 116: "Rasp"
     * 117: "Shadow Blast"
     * fighter[fighter_index].csmem is often used as the 0..7 index. This seems dangerous...
     */
    enum class eCombatSkill
    {
        kUnset = 0,

        kVenomousBite = 101,
        kDoubleSlash = 102,
        kChillTouch = 103,
        kFlashFlood = 104,
        kSweep = 105,
        kParaClaw = 106,
        kDragonBite = 107,
        kStoneGas = 108,
        kZemmelRod = 109,
        kPoisonGas = 110,
        kTangleRoot = 111,
        kPetrifyingBite = 112,
        kMaulOfTheTitans = 113,
        kStunningStrike = 114,
        kHowl = 115,
        kRasp = 116,
        kShadowBlast = 117,

        kTotal // always last
    };

    KFighter();
    ~KFighter();

    /** 
     * Determine whether the fighter's health is at-or-below a critical threshold.
     * 
     * @return true if fighter's health is severely low, false otherwise.
     */
    bool isFighterHealthCritical() const;

    /** 
     * Determine whether the fighter's magic is at-or-below a critical threshold.
     * 
     * @return true if fighter's magic is severely low, false otherwise.
     */
    bool isFighterMagicCritical() const;

private:
    int fighterHealthCriticalWarning;
    int fighterMagicCriticalWarning;

public:
    /** Name */
    std::string fighterName;

    /** Experience Points */
    int fighterExperience;

    /** Gold Points */
    int fighterMoney;

    /** Level */
    int fighterLevel;

    /** x-coord of image in datafile */
    int fighterImageDatafileX;

    /** y-coord of image in datafile */
    int fighterImageDatafileY;

    /** width in datafile */
    int fighterImageDatafileWidth;

    /** height in datafile */
    int fighterImageDatafileHeight;

    /** Hit Points */
    int fighterHealth;

    /** Max Hit Points */
    int fighterMaxHealth;

    /** Magic Points */
    int fighterMagic;

    /** Max Magic Points */
    int fighterMaxMagic;

    /** Defeat Item Probability: Probability in % that the enemy will yield an item when defeated. */
    int fighterDefeatItemProbability;

    /** Defeat Item Common: If the enemy yields an item, you will get this item 95% of the time. */
    int fighterDefeatItemCommon;

    /** Defeat Item Rare: If the enemy yields an item, you will get this item 5% of the time. */
    int fighterDefeatItemRare;

    /** Steal Item Common: If Ayla steals something, she will get this item 95% of the time. */
    int fighterStealItemCommon;

    /** Steal Item Rare: If Ayla steals something, she will get this item 5% of the time. */
    int fighterStealItemRare;

    /** See A_* constants in kq.h */
    int fighterStats[NUM_STATS];

    /** eResistance: See R_* constants */
    char fighterResistance[NUM_RES];

    /** Direction character's sprite faces */
    uint8_t fighterSpriteFacing;

    /** Battle sprite to display (standing, casting, attacking) */
    uint8_t fighterAttackSpriteFrame;

    /** Fighter frequently (or always) gets a "critical hit" buff (Sensar's "rage") */
    uint8_t fighterCanCriticalHit;

    /** eSpellType: Values > 0 mean that the fighter is affected (for good or bad) with this amount of each effect.
     *  Some, like S_DEAD, are only being used as a boolean to determine whether a fighter has been killed.
     */
    uint8_t fighterSpellEffectStats[NUM_SPELL_TYPES];

    /** Fighter will defend (instead of attack or use an item) for this turn during battle. */
    uint8_t fighterWillDefend;

    /** Combat skills: 1..99 seem to be player-owned; 100..253 appear to be enemy. 0, 254, 255: not specified.
     * 1..60 are found under the EMagic enum.
     * 101: "Venomous Bite"
     * 102: "Double Slash"
     * 103: "Chill Touch"
     * 104: "Flash Flood"
     * 105: "Sweep"
     * 106: "ParaClaw"
     * 107: "Dragon Bite"
     * 108: "Stone Gas"
     * 109: "Zemmel Rod"
     * 110: "Poison Gas"
     * 111: "Tangle Root"
     * 112: "Petrifying Bite"
     * 113: "Maul of the Titans"
     * 114: "Stunning Strike"
     * 115: "Howl"
     * 116: "Rasp"
     * 117: "Shadow Blast"
     * fighter[fighter_index].csmem is often used as the 0..7 index. This seems dangerous...
     */
    uint8_t fighterCombatSkill[8];
    uint8_t aip[8];
    uint8_t atrack[8];

    /** Spell number, associated with M_* spells, used within s_spell magic[] array. */
    uint32_t csmem;

    /** Spell target: who is going to be affected by the spell; can be set to -1 */
    int ctmem;

    /** Current Weapon Type: The shape of the currently held weapon (sword, dagger, axe etc) \sa hero_init() */
    uint32_t current_weapon_type;

    /** eResistance: Which Element type (sick, fire, water, etc.) */
    int welem;

    /** UNLiving (undead), like zombies, skeletons, etc. */
    int unl;

    int aux;
    int bonus;
    int bstat;
    int mrp;
    int imb_s;
    int imb_a;
    int imb[2];
    Raster* img;
};

extern KFighter fighter[NUM_FIGHTERS];
extern KFighter tempa, tempd;
