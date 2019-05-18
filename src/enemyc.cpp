/*! \file
 * \brief Enemy combat
 *
 * \author JB
 * \date ??????
 */

#include <memory>
#include <stdio.h>
#include <string.h>

#include "combat.h"
#include "draw.h"
#include "enemyc.h"
#include "eskill.h"
#include "gfx.h"
#include "heroc.h"
#include "imgcache.h"
#include "kq.h"
#include "magic.h"
#include "platform.h"
#include "random.h"
#include "res.h"
#include "selector.h"
#include "skills.h"

/*! Index related to enemies in an encounter */
int cf[NUM_FIGHTERS];

namespace KqFork
{
namespace EnemyC
{

/*  internal prototypes  */
void enemy_attack(size_t);
int enemy_cancast(size_t, size_t);
void enemy_curecheck(int);
void enemy_skillcheck(size_t fighterIndex, size_t skillNumber);
void enemy_spellcheck(size_t, size_t);
int enemy_stscheck(int, int);
void load_enemies();
KFighter* make_enemy(size_t enemyFighterIndex, KFighter* en);
int skill_setup(size_t fighterIndex, size_t skillNumber);
int spell_setup(int, int);

} // namespace EnemyC
} // namespace KqFork

/*! \page monster The Format of allstat.mon
 *
 * The format of allstat.mon is a space-separated sequence of rows.
 * Within a row, the column order is:
 *
 * -# Name
 * -# ignored (index number)
 * -# x-coord of image (in the datafile)
 * -# y-coord of image
 * -# width of image
 * -# height of image
 * -# xp
 * -# gold
 * -# level
 * -# max hp
 * -# max mp
 * -# dip Defeat Item Probability.
 * -# defeat_item_common Defeat Item Common
 * -# defeat_item_rare Defeat Item Rare
 * -# steal_item_common Steal Item Common
 * -# steal_item_rare Steal Item Rare
 * -# strength (agility and vitality are set to 0)
 * -# intelligence AND sagacity (both set to same)
 * -# stat[5] (A_SPD)
 * -# stat[6] (A_AUR)
 * -# stat[7] (A_SPI)
 * -# stat[8] (A_ATT)
 * -# stat[9] (A_HIT)
 * -# stat[10] (A_DEF)
 * -# stat[11] (A_EVD)
 * -# stat[12] (A_MAG)
 * -# bonus (bstat set to 0)
 * -# current_weapon_type (current weapon type)
 * -# welem Weapon elemental power
 * -# unl Undead Level (defense against Undead attacks)
 * -# crit (?)
 * -# imb_s Item for imbued spell
 * -# imb_a New value for SAG and INT when casting imbued.
 * -# imb[0] (?)
 * -# imb[1] (?)
 */

/**
 * Array of enemy 'fighters'  */
namespace KqFork
{
namespace EnemyC
{
std::vector<KFighter*> enemy_fighters;
} // namespace EnemyC
} // namespace KqFork

/**
 * Melee attack
 *
 * Do an enemy melee attack.  Enemies only defend if they are in critical
 * status.  This could use a little more smarts, so that more-intelligent
 * enemies would know to hit spellcasters or injured heroes first, and so
 * that berserk-type enemies don't defend.  The hero selection is done in
 * a different function, but it all starts here.
 *
 * \param   target_fighter_index Target
 */
void KqFork::EnemyC::enemy_attack(size_t target_fighter_index)
{
    int b, c;
    size_t fighter_index;

    if (fighter[target_fighter_index].fighterHealth < (fighter[target_fighter_index].fighterMaxHealth / 5) && fighter[target_fighter_index].fighterSpellEffectStats[S_CHARM] == 0)
    {
        if (kqrandom->random_range_exclusive(0, 4) == 0)
        {
            fighter[target_fighter_index].fighterWillDefend = 1;
            kqCombat.bIsEtherEffectActive[target_fighter_index] = false;
            return;
        }
    }
    if (fighter[target_fighter_index].fighterSpellEffectStats[S_CHARM] == 0)
    {
        b = auto_select_hero(target_fighter_index, NO_STS_CHECK);
    }
    else
    {
        if (fighter[target_fighter_index].ctmem == 0)
        {
            b = auto_select_hero(target_fighter_index, NO_STS_CHECK);
        }
        else
        {
            b = auto_select_enemy(target_fighter_index, NO_STS_CHECK);
        }
    }
    if (b < 0)
    {
        fighter[target_fighter_index].fighterWillDefend = 1;
        kqCombat.bIsEtherEffectActive[target_fighter_index] = false;
        return;
    }
    if ((uint32_t)b < MAX_PARTY_SIZE && numchrs > 1)
    {
        c = 0;
        for (fighter_index = 0; fighter_index < numchrs; fighter_index++)
        {
            if (activeAvatarIds[fighter_index] == TEMMIN && fighter[fighter_index].aux == 1)
            {
                c = fighter_index + 1;
            }
        }
        if (c != 0)
        {
            if (activeAvatarIds[b] != TEMMIN)
            {
                b = c - 1;
                fighter[c - 1].aux = 2;
            }
        }
    }
    kqCombat.fight(target_fighter_index, b, 0);
    kqCombat.bIsEtherEffectActive[target_fighter_index] = false;
}

/**
 * Check if enemy can cast this spell
 *
 * This function is fairly specific in that it will only
 * return 1 if the enemy has the spell in its list of spells,
 * is not mute, and has enough mp to cast the spell.
 *
 * \param   target_fighter_index Which enemy
 * \param   sp Spell to cast
 * \returns 1 if spell can be cast, 0 otherwise
 */
int KqFork::EnemyC::enemy_cancast(size_t target_fighter_index, size_t sp)
{
    size_t a;
    uint32_t enemyCombatSkillCount = 0;

    /* Enemy is mute; cannot cast the spell */
    if (fighter[target_fighter_index].fighterSpellEffectStats[S_MUTE] != 0)
    {
        return 0;
    }

    for (a = 0; a < 8; a++)
    {
        if (fighter[target_fighter_index].fighterCombatSkill[a] == sp)
        {
            enemyCombatSkillCount++;
        }
    }
    if (enemyCombatSkillCount == 0)
    {
        return 0;
    }
    if (fighter[target_fighter_index].fighterMagic < mp_needed(target_fighter_index, sp))
    {
        return 0;
    }
    return 1;
}

/**
 * Action for confused enemy
 *
 * Enemy actions are chosen differently if they are confused.  Confused
 * fighters either attack the enemy, an ally, or do nothing.  Confused
 * fighters never use spells or items.
 *
 * \sa auto_herochooseact()
 * \param   fighter_index Target
 */
void enemy_charmaction(size_t fighter_index)
{
    int a;

    if (!kqCombat.bIsEtherEffectActive[fighter_index])
    {
        return;
    }
    if (fighter[fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::IS_DEAD || fighter[fighter_index].fighterHealth <= 0)
    {
        kqCombat.bIsEtherEffectActive[fighter_index] = false;
        return;
    }
    for (a = 0; a < 5; a++)
    {
        if (fighter[fighter_index].atrack[a] > 0)
        {
            fighter[fighter_index].atrack[a]--;
        }
    }
    a = kqrandom->random_range_exclusive(0, 4);
    if (a == 0)
    {
        kqCombat.bIsEtherEffectActive[fighter_index] = false;
        return;
    }
    if (a == 1)
    {
        fighter[fighter_index].ctmem = 0;
        KqFork::EnemyC::enemy_attack(fighter_index);
        return;
    }
    fighter[fighter_index].ctmem = 1;
    KqFork::EnemyC::enemy_attack(fighter_index);
}

/**
 * Choose action for enemy
 *
 * There is the beginning of some intelligence to this... however, the
 * magic checking and skill checking functions aren't very smart yet :)
 * \todo PH would be good to have this script-enabled.
 *
 * \param   fighter_index Target action will be performed on
 */
void enemy_chooseaction(size_t fighter_index)
{
    int ap;
    size_t a;

    if (!kqCombat.bIsEtherEffectActive[fighter_index])
    {
        return;
    }
    if (fighter[fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::IS_DEAD || fighter[fighter_index].fighterHealth <= 0)
    {
        kqCombat.bIsEtherEffectActive[fighter_index] = false;
        return;
    }

    for (a = 0; a < 8; a++)
    {
        if (fighter[fighter_index].atrack[a] > 0)
        {
            fighter[fighter_index].atrack[a]--;
        }
    }
    fighter[fighter_index].fighterWillDefend = 0;
    fighter[fighter_index].fighterSpriteFacing = 1;
    if (fighter[fighter_index].fighterHealth < fighter[fighter_index].fighterMaxHealth * 2 / 3 && kqrandom->random_range_exclusive(0, 100) < 50 && fighter[fighter_index].fighterSpellEffectStats[S_MUTE] == 0)
    {
        KqFork::EnemyC::enemy_curecheck(fighter_index);
        if (!kqCombat.bIsEtherEffectActive[fighter_index])
        {
            return;
        }
    }

    ap = kqrandom->random_range_exclusive(0, 100);
    for (a = 0; a < 8; a++)
    {
        if (ap < fighter[fighter_index].aip[a])
        {
            if (fighter[fighter_index].fighterCombatSkill[a] >= 100 && fighter[fighter_index].fighterCombatSkill[a] < 254)
            {
                KqFork::EnemyC::enemy_skillcheck(fighter_index, a);
                if (!kqCombat.bIsEtherEffectActive[fighter_index])
                {
                    return;
                }
                else
                {
                    ap = fighter[fighter_index].aip[a] + 1;
                }
            }
            if (fighter[fighter_index].fighterCombatSkill[a] > 0 && fighter[fighter_index].fighterCombatSkill[a] < 100 && fighter[fighter_index].fighterSpellEffectStats[S_MUTE] == 0)
            {
                KqFork::EnemyC::enemy_spellcheck(fighter_index, a);
                if (!kqCombat.bIsEtherEffectActive[fighter_index])
                {
                    return;
                }
                else
                {
                    ap = fighter[fighter_index].aip[a] + 1;
                }
            }
        }
    }
    KqFork::EnemyC::enemy_attack(fighter_index);
    kqCombat.bIsEtherEffectActive[fighter_index] = false;
}

/**
 * Use cure spell
 *
 * If the caster has a cure/drain spell, use it to cure itself.
 *
 * \param   w Caster
 */
void KqFork::EnemyC::enemy_curecheck(int w)
{
    int a;

    a = -1;
    if (KqFork::EnemyC::enemy_cancast(w, M_DRAIN) == 1)
    {
        a = M_DRAIN;
    }
    if (KqFork::EnemyC::enemy_cancast(w, M_CURE1) == 1)
    {
        a = M_CURE1;
    }
    if (KqFork::EnemyC::enemy_cancast(w, M_CURE2) == 1)
    {
        a = M_CURE2;
    }
    if (KqFork::EnemyC::enemy_cancast(w, M_CURE3) == 1)
    {
        a = M_CURE3;
    }
    if (KqFork::EnemyC::enemy_cancast(w, M_CURE4) == 1)
    {
        a = M_CURE4;
    }
    if (a != -1)
    {
        fighter[w].csmem = a;
        fighter[w].ctmem = w;
        combat_spell(w, 0);
        kqCombat.bIsEtherEffectActive[w] = false;
    }
}

/**
 * Initialize enemy & sprites for combat
 *
 * If required, load the all the enemies, then
 * init the ones that are going into battle, by calling make_enemy() and
 * copying the graphics sprites into cframes[] and tcframes[].
 * Looks at the cf[] array to see which enemies to do.
 *
 * \author PH
 * \date 2003????
 */
void enemy_init()
{
    size_t fighter_index, frame_index;

    if (KqFork::EnemyC::enemy_fighters.size() == 0)
    {
        KqFork::EnemyC::load_enemies();
    }
    for (fighter_index = 0; fighter_index < kqCombat.num_enemies; ++fighter_index)
    {
        size_t enemyFighterIndex = cf[fighter_index];
        KFighter* f = nullptr;
        if (enemyFighterIndex < KqFork::EnemyC::enemy_fighters.size() && KqFork::EnemyC::enemy_fighters.size() > 0)
        {
            f = KqFork::EnemyC::enemy_fighters[enemyFighterIndex];
            fighter[fighter_index + MAX_PARTY_SIZE] = *f;
        }
        if (f == nullptr)
        {
            return;
        }

        for (frame_index = 0; frame_index < MAXCFRAMES; ++frame_index)
        {
            /* If, in a previous combat, we made a bitmap, destroy it now */
            if (cframes[fighter_index + MAX_PARTY_SIZE][frame_index])
            {
                delete (cframes[fighter_index + MAX_PARTY_SIZE][frame_index]);
            }
            /* and create a new one */
            cframes[fighter_index + MAX_PARTY_SIZE][frame_index] = new Raster(f->img->width, f->img->height);
            blit(f->img, cframes[fighter_index + MAX_PARTY_SIZE][frame_index], 0, 0, 0, 0, f->img->width, f->img->height);
            tcframes[fighter_index + MAX_PARTY_SIZE][frame_index] = kqDraw.copy_bitmap(tcframes[fighter_index + MAX_PARTY_SIZE][frame_index], f->img);
        }
    }
}

/**
 * Check skills
 *
 * Very simple... see if the skill that was selected can be used.
 *
 * \param   w Enemy index
 * \param   ws Enemy skill index
 */
void KqFork::EnemyC::enemy_skillcheck(size_t fighterIndex, size_t skillNumber)
{
    if (fighterIndex >= NUM_FIGHTERS || skillNumber >= 8)
    {
        return;
    }
    uint8_t fighterCombatSkill = fighter[fighterIndex].fighterCombatSkill[skillNumber];

    if (fighterCombatSkill > 100 && fighterCombatSkill < 254)
    {
        if (fighterCombatSkill == 105/*Sweep*/)
        {
            if (numchrs == 1)
            {
                fighter[fighterIndex].atrack[skillNumber] = 1;
            }
            if (numchrs == 2 && (fighter[0].fighterSpellEffectStats[S_DEAD] == eDeathType::IS_DEAD || fighter[1].fighterSpellEffectStats[S_DEAD] == eDeathType::IS_DEAD))
            {
                fighter[fighterIndex].atrack[skillNumber] = 1;
            }
        }
        if (fighter[fighterIndex].atrack[skillNumber] == 0 && KqFork::EnemyC::skill_setup(fighterIndex, skillNumber) == 1)
        {
            combat_skill(fighterIndex);
            kqCombat.bIsEtherEffectActive[fighterIndex] = false;
        }
    }
}

/**
 * Check selected spell
 *
 * This function looks at the enemy's selected spell and tries to
 * determine whether to bother casting it or not.
 *
 * \param   attack_fighter_index Caster
 * \param   defend_fighter_index Target
 */
void KqFork::EnemyC::enemy_spellcheck(size_t attack_fighter_index, size_t defend_fighter_index)
{
    int fighterCombatSkill = 0, aux, yes = 0;
    size_t fighter_index;

    uint8_t ai = fighter[attack_fighter_index].fighterCombatSkill[defend_fighter_index];
    if (ai > 0 && ai < 100)
    {
        fighterCombatSkill = ai;
        if (fighterCombatSkill > 0 && KqFork::EnemyC::enemy_cancast(attack_fighter_index, fighterCombatSkill) == 1)
        {
            switch (fighterCombatSkill)
            {
            case M_SHIELD:
            case M_SHIELDALL:
                yes = KqFork::EnemyC::enemy_stscheck(S_SHIELD, MAX_PARTY_SIZE);
                break;
            case M_HOLYMIGHT:
                aux = 0;
                for (fighter_index = MAX_PARTY_SIZE; fighter_index < MAX_PARTY_SIZE + kqCombat.num_enemies; fighter_index++)
                {
                    if (fighter[fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD && fighter[fighter_index].fighterSpellEffectStats[S_STRENGTH] < 2)
                    {
                        aux++;
                    }
                }
                if (aux > 0)
                {
                    yes = 1;
                }
                break;
            case M_BLESS:
                aux = 0;
                for (fighter_index = MAX_PARTY_SIZE; fighter_index < MAX_PARTY_SIZE + kqCombat.num_enemies; fighter_index++)
                {
                    if (fighter[fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD && fighter[fighter_index].fighterSpellEffectStats[S_BLESS] < 3)
                    {
                        aux++;
                    }
                }
                if (aux > 0)
                {
                    yes = 1;
                }
                break;
            case M_TRUEAIM:
                yes = KqFork::EnemyC::enemy_stscheck(S_TRUESHOT, MAX_PARTY_SIZE);
                break;
            case M_REGENERATE:
                yes = KqFork::EnemyC::enemy_stscheck(S_REGEN, MAX_PARTY_SIZE);
                break;
            case M_THROUGH:
                yes = KqFork::EnemyC::enemy_stscheck(S_ETHER, MAX_PARTY_SIZE);
                break;
            case M_HASTEN:
            case M_QUICKEN:
                aux = 0;
                for (fighter_index = MAX_PARTY_SIZE; fighter_index < MAX_PARTY_SIZE + kqCombat.num_enemies; fighter_index++)
                {
                    if (fighter[fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD && fighter[fighter_index].fighterSpellEffectStats[S_TIME] != 2)
                    {
                        aux++;
                    }
                }
                if (aux > 0)
                {
                    yes = 1;
                }
                break;
            case M_SHELL:
            case M_WALL:
                yes = KqFork::EnemyC::enemy_stscheck(S_RESIST, MAX_PARTY_SIZE);
                break;
            case M_ABSORB:
                if (fighter[attack_fighter_index].fighterHealth < fighter[attack_fighter_index].fighterMaxHealth / 2)
                {
                    yes = 1;
                }
                break;
            case M_VENOM:
            case M_BLIND:
            case M_CONFUSE:
            case M_HOLD:
            case M_STONE:
            case M_SILENCE:
            case M_SLEEP:
                yes = KqFork::EnemyC::enemy_stscheck(magic[fighterCombatSkill].elem - 8, 0);
                break;
            case M_NAUSEA:
            case M_MALISON:
                yes = KqFork::EnemyC::enemy_stscheck(S_MALISON, 0);
                break;
            case M_SLOW:
                aux = 0;
                for (fighter_index = 0; fighter_index < numchrs; fighter_index++)
                {
                    if (fighter[fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD && fighter[fighter_index].fighterSpellEffectStats[S_TIME] != 1)
                    {
                        aux++;
                    }
                }
                if (aux > 0)
                {
                    yes = 1;
                }
                break;
            case M_SLEEPALL:
                aux = 0;
                for (fighter_index = 0; fighter_index < numchrs; fighter_index++)
                {
                    if (fighter[fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD && fighter[fighter_index].fighterSpellEffectStats[S_SLEEP] == 0)
                    {
                        aux++;
                    }
                }
                if (aux > 0)
                {
                    yes = 1;
                }
                break;
            case M_DIVINEGUARD:
                aux = 0;
                for (fighter_index = MAX_PARTY_SIZE; fighter_index < MAX_PARTY_SIZE + kqCombat.num_enemies; fighter_index++)
                {
                    if (fighter[fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD && fighter[fighter_index].fighterSpellEffectStats[S_SHIELD] == 0 && fighter[fighter_index].fighterSpellEffectStats[S_RESIST] == 0)
                    {
                        aux++;
                    }
                }
                if (aux > 0)
                {
                    yes = 1;
                }
                break;
            case M_DOOM:
                aux = 0;
                for (fighter_index = 0; fighter_index < numchrs; fighter_index++)
                {
                    if (fighter[fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD && fighter[fighter_index].fighterHealth >= fighter[fighter_index].fighterMaxHealth / 3)
                    {
                        aux++;
                    }
                }
                if (aux > 0)
                {
                    yes = 1;
                }
                break;
            case M_DRAIN:
                if (fighter[attack_fighter_index].fighterHealth < fighter[attack_fighter_index].fighterMaxHealth)
                {
                    yes = 1;
                }
                break;
            default:
                yes = 1;
                break;
            }
        }
    }
    if (yes == 0)
    {
        return;
    }
    if (KqFork::EnemyC::spell_setup(attack_fighter_index, fighterCombatSkill) == 1)
    {
        combat_spell(attack_fighter_index, 0);
        kqCombat.bIsEtherEffectActive[attack_fighter_index] = false;
    }
}

/**
 * Check status
 *
 * Checks a passed status condition to see if anybody is affected by it and
 * determines whether it should be cast or not
 *
 * \param   ws Which stat to consider
 * \param   s Starting target for multiple targets
 */
int KqFork::EnemyC::enemy_stscheck(int ws, int s)
{
    uint32_t fighter_affected = 0;
    size_t fighter_index;

    if (s == MAX_PARTY_SIZE)
    {
        for (fighter_index = MAX_PARTY_SIZE; fighter_index < MAX_PARTY_SIZE + kqCombat.num_enemies; fighter_index++)
        {
            if (fighter[fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD && fighter[fighter_index].fighterSpellEffectStats[ws] == 0)
            {
                fighter_affected++;
            }
        }
        if (fighter_affected > 0)
        {
            return 1;
        }
    }
    else
    {
        for (fighter_index = 0; fighter_index < numchrs; fighter_index++)
        {
            if (fighter[fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD && fighter[fighter_index].fighterSpellEffectStats[ws] == 0)
            {
                fighter_affected++;
            }
        }
        if (fighter_affected > 0)
        {
            return 1;
        }
    }
    return 0;
}

//static void dump_en() {
//    extern int save_fighters(const char* filename, KFighter* fighters, int count);
//    std::unique_ptr<KFighter[]> tmp(new KFighter[KqFork::EnemyC::enemies_n]);
//    for (int i = 0; i < KqFork::EnemyC::enemies_n; ++i) {
//        tmp[i] = KqFork::EnemyC::enemy_fighters[i];
//    }
//    save_fighters("save-f.xml", tmp.get(), KqFork::EnemyC::enemies_n);
//}

/**
 * Load all enemies from disk
 *
 * Loads up enemies from the *.mon files and fills the enemies[] array.
 * \author PH
 * \date 2003????
 */
void KqFork::EnemyC::load_enemies()
{
    int tmp, lx, ly, p;
    FILE* edat = nullptr;

    if (enemy_fighters.size() != 0)
    {
        /* Already done the loading */
        return;
    }

    Raster* enemy_gfx = get_cached_image("enemy.png");

    if (!enemy_gfx)
    {
        Game.program_death(_("Could not load enemy sprites!"));
    }
    edat = fopen(kqres(DATA_DIR, "allstat.mon").c_str(), "r");
    if (!edat)
    {
        Game.program_death(_("Could not load 1st enemy datafile!"));
    }

    for (KFighter* enemy_fighter : enemy_fighters)
    {
        delete enemy_fighter;
    }
    enemy_fighters.clear();

    // Loop through for every monster in allstat.mon
    while (fscanf(edat, "%s", strbuf) != EOF)
    {
        KFighter* f = new KFighter;
        enemy_fighters.push_back(f);

        // Enemy name
        f->fighterName = strbuf;
        // Index number (ignored; automatically generated)
        fscanf(edat, "%d", &tmp);
        // x-coord of image in datafile
        fscanf(edat, "%d", &tmp);
        lx = tmp;
        // y-coord of image in datafile
        fscanf(edat, "%d", &tmp);
        ly = tmp;
        // Image width
        fscanf(edat, "%d", &tmp);
        f->fighterImageDatafileWidth = tmp;
        // Image length (height)
        fscanf(edat, "%d", &tmp);
        f->fighterImageDatafileHeight = tmp;
        // Experience points earned
        fscanf(edat, "%d", &tmp);
        f->fighterExperience = tmp;
        // Gold received
        fscanf(edat, "%d", &tmp);
        f->fighterMoney = tmp;
        // Level
        fscanf(edat, "%d", &tmp);
        f->fighterLevel = tmp;
        // Max HP
        fscanf(edat, "%d", &tmp);
        f->fighterMaxHealth = tmp;
        // Max MP
        fscanf(edat, "%d", &tmp);
        f->fighterMaxMagic = tmp;
        // Defeat Item Probability: chance of finding any items after defeat
        fscanf(edat, "%d", &tmp);
        f->fighterDefeatItemProbability = tmp;
        // Defeat Item Common: item found commonly of the time
        fscanf(edat, "%d", &tmp);
        f->fighterDefeatItemCommon = tmp;
        // Defeat Item Rare: item found rarely
        fscanf(edat, "%d", &tmp);
        f->fighterDefeatItemRare = tmp;
        // Steal Item Common: item found commonly from stealing
        fscanf(edat, "%d", &tmp);
        f->fighterStealItemCommon = tmp;
        // Steal Item Rare: item found rarely when stealing
        fscanf(edat, "%d", &tmp);
        f->fighterStealItemRare = tmp;
        // Enemy's strength (agility & vitality set to zero)
        fscanf(edat, "%d", &tmp);
        f->fighterStats[A_STR] = tmp;
        f->fighterStats[A_AGI] = 0;
        f->fighterStats[A_VIT] = 0;
        // Intelligence & Sagacity (both the same)
        fscanf(edat, "%d", &tmp);
        f->fighterStats[A_INT] = tmp;
        f->fighterStats[A_SAG] = tmp;
        // Defense against: Speed, Spirit, Attack, Hit, Defense, Evade, Magic (in that order)
        for (p = 5; p < 13; p++)
        {
            fscanf(edat, "%d", &tmp);
            f->fighterStats[p] = tmp;
        }
        // Bonus
        fscanf(edat, "%d", &tmp);
        f->bonus = tmp;
        f->bstat = 0;
        // Current weapon type
        fscanf(edat, "%d", &tmp);
        f->current_weapon_type = (uint32_t)tmp;
        // Weapon elemental type
        fscanf(edat, "%d", &tmp);
        f->welem = tmp;
        // Undead Level (defense against Undead attacks)
        fscanf(edat, "%d", &tmp);
        f->unl = tmp;
        // Critical attacks
        fscanf(edat, "%d", &tmp);
        f->fighterCanCriticalHit = tmp;
        // Temp Sag & Int for Imbued
        fscanf(edat, "%d", &tmp);
        f->imb_s = tmp;
        // Imbued stat type (Spd, Spi, Att, Hit, Def, Evd, Mag)
        fscanf(edat, "%d", &tmp);
        f->imb_a = tmp;
        f->img = new Raster(f->fighterImageDatafileWidth, f->fighterImageDatafileHeight);
        enemy_gfx->blitTo(f->img, lx, ly, 0, 0, f->fighterImageDatafileWidth, f->fighterImageDatafileHeight);
        for (p = 0; p < 2; p++)
        {
            fscanf(edat, "%d", &tmp);
            f->imb[p] = tmp;
        }
    }
    fclose(edat);
    edat = fopen(kqres(DATA_DIR, "resabil.mon").c_str(), "r");
    if (!edat)
    {
        Game.program_death(_("Could not load 2nd enemy datafile!"));
    }
    for (size_t i = 0; i < enemy_fighters.size(); i++)
    {
        KFighter* f = enemy_fighters[i];
        fscanf(edat, "%s", strbuf);
        fscanf(edat, "%d", &tmp);
        for (p = 0; p < R_TOTAL_RES; p++)
        {
            fscanf(edat, "%d", &tmp);
            f->fighterResistance[p] = tmp;
        }
        for (p = 0; p < 8; p++)
        {
            fscanf(edat, "%d", &tmp);
            f->fighterCombatSkill[p] = tmp;
        }
        for (p = 0; p < 8; p++)
        {
            fscanf(edat, "%d", &tmp);
            f->aip[p] = tmp;
            f->atrack[p] = 0;
        }
        f->fighterHealth = f->fighterMaxHealth;
        f->fighterMagic = f->fighterMaxMagic;
        for (p = 0; p < NUM_SPELL_TYPES; p++)
        {
            f->fighterSpellEffectStats[p] = 0;
        }
        f->aux = 0;
        f->mrp = 100;
    }
    fclose(edat);
    //dump_en();
}

/**
 * Prepare an enemy for battle
 *
 * Fills out a supplied KFighter structure with the default,
 * starting values for an enemy.
 * \author PH
 * \date 2003????
 * \param   who The numeric id of the enemy to make
 * \param   en Pointer to a KFighter object to initialize
 * \returns the value of en, for convenience, or NULL if an error occurred.
 * \sa make_enemy_by_name()
 */
KFighter* KqFork::EnemyC::make_enemy(size_t enemyFighterIndex, KFighter* en)
{
    KFighter* fighterPtr = nullptr;
    if (KqFork::EnemyC::enemy_fighters.size() > 0 && enemyFighterIndex < KqFork::EnemyC::enemy_fighters.size())
    {
        *en = *KqFork::EnemyC::enemy_fighters[enemyFighterIndex];
        return en;
    }
    else
    {
        /* PH probably should call program_death() here? */
        return nullptr;
    }
}

/**
 * Enemy initialization
 *
 * This is the main enemy initialization routine.  This function sets up
 * the enemy types and then loads each one in.  It also calls a helper
 * function or two to complete the process.
 *
 * The encounter table consists of several 'sub-tables', grouped by
 * encounter number. Each row is one possible battle.
 * Fills in the cf[] array of enemies to load.
 *
 * \param   en Encounter number in the Encounter table.
 * \param   etid If =99, select a random row with that encounter number,
 *          otherwise select row etid.
 * \returns number of random encounter
 */
size_t select_encounter(int en, int etid)
{
    size_t i, p, j;
    int stop = 0, where = 0;
    bool entryFound = false;
    size_t entry = 0;

    while (!stop)
    {
        if (erows[where].tnum == en)
        {
            stop = 1;
        }
        else
        {
            where++;
        }
        if (where >= NUM_ETROWS)
        {
            sprintf(strbuf, _("There are no rows for encounter table #%d!"), en);
            Game.program_death(strbuf);
        }
    }
    if (etid == 99)
    {
        i = kqrandom->random_range_exclusive(1, 101);
        while (!entryFound)
        {
            if (i <= erows[where].per)
            {
                entry = where;
                entryFound = true;
            }
            else
            {
                where++;
            }
            if (erows[where].tnum > en || where >= NUM_ETROWS)
            {
                Game.program_death(_("Couldn't select random encounter table row!"));
            }
        }
    }
    else
    {
        entry = where + etid;
    }
    p = 0;
    for (j = 0; j < 5; j++)
    {
        if (erows[entry].idx[j] > 0)
        {
            cf[p] = erows[entry].idx[j] - 1;
            p++;
        }
    }
    kqCombat.num_enemies = p;
    /* adjust 'too hard' combat where player is alone and faced by >2 enemies */
    if (kqCombat.num_enemies > 2 && numchrs == 1 && erows[entry].lvl + 2 > party[activeAvatarIds[0]].lvl && etid == 99)
    {
        kqCombat.num_enemies = 2;
    }
    if (kqCombat.num_enemies == 0)
    {
        Game.program_death(_("Empty encounter table row!"));
    }
    return entry;
}

/**
 * Set up skill targets
 *
 * This is just for aiding in skill setup... choosing skill targets.
 *
 * \param   fighterIndex Caster
 * \param   skillNumber Which skill
 * \returns 1 for success, 0 otherwise
 */
int KqFork::EnemyC::skill_setup(size_t fighterIndex, size_t skillNumber)
{
    if (fighterIndex >= NUM_FIGHTERS || skillNumber >= 8)
    {
        return 0;
    }
    int fighterCombatSkill = fighter[fighterIndex].fighterCombatSkill[skillNumber];

    fighter[fighterIndex].csmem = skillNumber;
    if (fighterCombatSkill == 101/*"Venomous Bite"*/ ||
        fighterCombatSkill == 102/*"Double Slash"*/ ||
        fighterCombatSkill == 103/*"Chill Touch"*/ ||
        fighterCombatSkill == 106/*"ParaClaw"*/ ||
        fighterCombatSkill == 107/*"Dragon Bite"*/ ||
        fighterCombatSkill == 112/*"Petrifying Bite"*/ ||
        fighterCombatSkill == 114/*"Stunning Strike"*/)
    {
        fighter[fighterIndex].ctmem = auto_select_hero(fighterIndex, NO_STS_CHECK);
        if (fighter[fighterIndex].ctmem == -1)
        {
            return 0;
        }
        return 1;
    }
    else
    {
        fighter[fighterIndex].ctmem = SEL_ALL_ENEMIES;
        return 1;
    }
    return 0;
}

/**
 * Helper for casting
 *
 * This is just a helper function for setting up the casting of a spell
 * by an enemy.
 *
 * \param   whom Caster
 * \param   z Which spell will be cast
 * \returns 0 if spell ineffective, 1 otherwise
 */
int KqFork::EnemyC::spell_setup(int whom, int z)
{
    int zst = NO_STS_CHECK, aux;
    size_t fighter_index;

    switch (z)
    {
    case M_SHIELD:
        zst = S_SHIELD;
        break;
    case M_HOLYMIGHT:
        zst = S_STRENGTH;
        break;
    case M_SHELL:
    case M_WALL:
        zst = S_RESIST;
        break;
    case M_VENOM:
    case M_HOLD:
    case M_BLIND:
    case M_SILENCE:
    case M_SLEEP:
    case M_CONFUSE:
    case M_STONE:
        zst = magic[z].elem - 8;
        break;
    case M_NAUSEA:
        zst = S_MALISON;
        break;
    }
    fighter[whom].csmem = z;
    fighter[whom].ctmem = -1;
    switch (magic[z].tgt)
    {
    case TGT_ALLY_ONE:
        fighter[whom].ctmem = auto_select_enemy(whom, zst);
        break;
    case TGT_ALLY_ALL:
        fighter[whom].ctmem = SEL_ALL_ALLIES;
        break;
    case TGT_ALLY_ONEALL:
        if (z == M_CURE1 || z == M_CURE2 || z == M_CURE3 || z == M_CURE4)
        {
            aux = 0;
            for (fighter_index = MAX_PARTY_SIZE; fighter_index < MAX_PARTY_SIZE + kqCombat.num_enemies; fighter_index++)
            {
                if (fighter[fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD && fighter[fighter_index].fighterHealth < fighter[fighter_index].fighterMaxHealth * 75 / 100)
                {
                    aux++;
                }
            }
            if (aux > 1)
            {
                fighter[whom].ctmem = SEL_ALL_ALLIES;
            }
            else
            {
                fighter[whom].ctmem = auto_select_enemy(whom, CURE_CHECK);
            }
        }
        else
        {
            if (kqrandom->random_range_exclusive(0, 4) < 2)
            {
                fighter[whom].ctmem = SEL_ALL_ALLIES;
            }
            else
            {
                fighter[whom].ctmem = auto_select_enemy(whom, CURE_CHECK);
            }
        }
        break;
    case TGT_ENEMY_ONE:
        fighter[whom].ctmem = auto_select_hero(whom, zst);
        break;
    case TGT_ENEMY_ALL:
        fighter[whom].ctmem = SEL_ALL_ENEMIES;
        break;
    case TGT_ENEMY_ONEALL:
        if (kqrandom->random_range_exclusive(0, 4) < 3)
        {
            fighter[whom].ctmem = SEL_ALL_ENEMIES;
        }
        else
        {
            fighter[whom].ctmem = auto_select_hero(whom, NO_STS_CHECK);
        }
        break;
    }
    if (fighter[whom].ctmem == -1)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}
