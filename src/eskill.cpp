#include <stdio.h>
#include <string.h>

#include "combat.h"
#include "draw.h"
#include "effects.h"
#include "enemyc.h"
#include "eskill.h"
#include "heroc.h"
#include "itemmenu.h"
#include "kq.h"
#include "magic.h"
#include "masmenu.h"
#include "menu.h"
#include "random.h"
#include "res.h"
#include "setup.h"

/*! \file
 * \brief Enemy skills
 */

/*! \brief Process each enemy skill
 *
 * Just a function to process each enemy skill by index number.
 *
 * \param   fighter_index Index of attacker
 */
void combat_skill(size_t fighter_index)
{
    if (fighter_index >= NUM_FIGHTERS)
    {
        return;
    }

    if (fighter[fighter_index].csmem >= 8)
    {
        return;
    }
    int fighterCombatSkill = fighter[fighter_index].fighterCombatSkill[fighter[fighter_index].csmem];
    int tgt = fighter[fighter_index].ctmem;
    size_t target_fighter_index;
    size_t affected_targets;
    int b;

    tempa = status_adjust(fighter_index);
    kqCombat.battle_render(0, 0, 0);
    kqDraw.blit2screen(0, 0);
    switch (fighterCombatSkill)
    {
    case 101:
        strcpy(attack_string, _("Venomous Bite"));
        display_attack_string = 1;
        tempa.welem = R_POISON + 1;
        kqCombat.fight(fighter_index, tgt, 1);
        display_attack_string = false;
        fighter[fighter_index].atrack[fighter[fighter_index].csmem] = 2;
        break;
    case 102:
        strcpy(attack_string, _("Double Slash"));
        display_attack_string = 1;
        tempa.fighterStats[A_ATT] = tempa.fighterStats[A_ATT] * 15 / 10;
        kqCombat.fight(fighter_index, tgt, 1);
        display_attack_string = false;
        fighter[fighter_index].atrack[fighter[fighter_index].csmem] = 2;
        break;
    case 103:
        strcpy(attack_string, _("Chill Touch"));
        display_attack_string = 1;
        draw_spellsprite(tgt, 0, 10, 1);
        special_damage_oneall_enemies(fighter_index, 60, R_ICE, tgt, 0);
        display_attack_string = false;
        fighter[fighter_index].atrack[fighter[fighter_index].csmem] = 2;
        break;
    case 104:
        strcpy(attack_string, _("Flash Flood"));
        display_attack_string = 1;
        draw_hugesprite(0, 80, 108, 21, 1);
        /*  dudaskank suggest replacing 999 with SEL_ALL_ENEMIES  */
        special_damage_oneall_enemies(fighter_index, 40, R_ICE, SEL_ALL_ENEMIES, 1);
        display_attack_string = false;
        fighter[fighter_index].atrack[fighter[fighter_index].csmem] = 3;
        break;
    case 105:
        affected_targets = 0;
        for (target_fighter_index = 0; target_fighter_index < numchrs; target_fighter_index++)
        {
            if (fighter[target_fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD)
            {
                affected_targets++;
            }
        }
        if (affected_targets > 1)
        {
            fighter[fighter_index].ctmem = 1000;
        }
        strcpy(attack_string, _("Sweep"));
        display_attack_string = 1;
        tempa.fighterStats[A_ATT] = tempa.fighterStats[A_ATT] * 75 / 100;
        kqCombat.multi_fight(fighter_index);
        display_attack_string = false;
        fighter[fighter_index].atrack[fighter[fighter_index].csmem] = 2;
        break;
    case 106:
        strcpy(attack_string, _("ParaClaw"));
        display_attack_string = 1;
        tempa.welem = R_PARALYZE + 1;
        kqCombat.fight(fighter_index, tgt, 1);
        display_attack_string = false;
        fighter[fighter_index].atrack[fighter[fighter_index].csmem] = 3;
        break;
    case 107:
        strcpy(attack_string, _("Dragon Bite"));
        display_attack_string = 1;
        tempa.fighterStats[A_ATT] = tempa.fighterStats[A_ATT] * 15 / 10;
        tempa.fighterStats[A_HIT] = tempa.fighterStats[A_HIT] * 9 / 10;
        tempa.welem = 0;
        kqCombat.fight(fighter_index, tgt, 1);
        display_attack_string = false;
        fighter[fighter_index].atrack[fighter[fighter_index].csmem] = 2;
        break;
    case 108:
        affected_targets = 0;
        strcpy(attack_string, _("Stone Gas"));
        draw_spellsprite(0, 1, 46, 1);
        for (target_fighter_index = 0; target_fighter_index < numchrs; target_fighter_index++)
        {
            if (fighter[target_fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD)
            {
                if (res_throw(target_fighter_index, R_PETRIFY) == 0 && non_dmg_save(target_fighter_index, 75) == 0)
                {
                    fighter[target_fighter_index].fighterSpellEffectStats[S_STONE] = kqrandom->random_range_exclusive(2, 5);
                    kqCombat.ta[target_fighter_index] = NODISPLAY;
                }
                else
                {
                    kqCombat.ta[target_fighter_index] = MISS;
                    affected_targets++;
                }
            }
        }
        if (affected_targets > 0)
        {
            display_amount(0, eFont::FONT_WHITE, 1);
        }
        fighter[fighter_index].atrack[fighter[fighter_index].csmem] = 3;
        break;
    case 109:
        affected_targets = 0;
        strcpy(attack_string, _("Zemmel Rod"));
        if (kqrandom->random_range_exclusive(0, 4) < 2)
        {
            draw_spellsprite(0, 1, 11, 1);
            /*  dudaskank suggest replacing 999 with SEL_ALL_ENEMIES  */
            special_damage_oneall_enemies(fighter_index, 25, R_THUNDER, SEL_ALL_ENEMIES, 1);
            fighter[fighter_index].atrack[fighter[fighter_index].csmem] = 2;
            return;
        }
        draw_spellsprite(0, 1, 40, 0);
        for (target_fighter_index = 0; target_fighter_index < numchrs; target_fighter_index++)
        {
            if (res_throw(target_fighter_index, R_TIME) == 0)
            {
                if (non_dmg_save(target_fighter_index, 75) == 0 && fighter[target_fighter_index].fighterSpellEffectStats[S_STONE] == 0)
                {
                    if (fighter[target_fighter_index].fighterSpellEffectStats[S_TIME] == 2)
                    {
                        fighter[target_fighter_index].fighterSpellEffectStats[S_TIME] = 0;
                    }
                    else
                    {
                        if (fighter[target_fighter_index].fighterSpellEffectStats[S_TIME] == 0)
                        {
                            fighter[target_fighter_index].fighterSpellEffectStats[S_TIME] = 1;
                            kqCombat.ta[target_fighter_index] = NODISPLAY;
                        }
                        else
                        {
                            kqCombat.ta[target_fighter_index] = MISS;
                            affected_targets++;
                        }
                    }
                }
                else
                {
                    kqCombat.ta[target_fighter_index] = MISS;
                    affected_targets++;
                }
            }
            else
            {
                kqCombat.ta[target_fighter_index] = MISS;
                affected_targets++;
            }
        }
        if (affected_targets > 0)
        {
            display_amount(0, eFont::FONT_WHITE, 1);
        }
        fighter[fighter_index].atrack[fighter[fighter_index].csmem] = 2;
        break;
    case 110:
        strcpy(attack_string, _("Poison Gas"));
        draw_spellsprite(0, 1, 47, 1);
        /*  dudaskank suggest replacing 999 with SEL_ALL_ENEMIES  */
        special_damage_oneall_enemies(fighter_index, 40, R_POISON, SEL_ALL_ENEMIES, 1);
        fighter[fighter_index].atrack[fighter[fighter_index].csmem] = 3;
        break;
    case 111:
        affected_targets = 0;
        strcpy(attack_string, _("Tangle Root"));
        draw_spellsprite(0, 1, 24, 0);
        for (target_fighter_index = 0; target_fighter_index < numchrs; target_fighter_index++)
        {
            if (res_throw(target_fighter_index, S_STOP) == 0 && non_dmg_save(target_fighter_index, 65) == 0 && fighter[target_fighter_index].fighterSpellEffectStats[S_STONE] == 0)
            {
                fighter[target_fighter_index].fighterSpellEffectStats[S_STOP] = kqrandom->random_range_exclusive(2, 4);
                kqCombat.ta[target_fighter_index] = NODISPLAY;
            }
            else
            {
                kqCombat.ta[target_fighter_index] = MISS;
                affected_targets++;
            }
        }
        if (affected_targets > 0)
        {
            display_amount(0, eFont::FONT_WHITE, 1);
        }
        fighter[fighter_index].atrack[fighter[fighter_index].csmem] = 3;
        break;
    case 112:
        strcpy(attack_string, _("Petrifying Bite"));
        display_attack_string = 1;
        tempa.fighterStats[A_ATT] = tempa.fighterStats[A_ATT];
        tempa.fighterStats[A_HIT] = tempa.fighterStats[A_HIT] * 8 / 10;
        tempa.welem = R_PETRIFY + 1;
        kqCombat.fight(fighter_index, tgt, 1);
        display_attack_string = false;
        fighter[fighter_index].atrack[fighter[fighter_index].csmem] = 3;
        break;
    case 113:
        strcpy(attack_string, _("Maul of the Titans"));
        draw_hugesprite(0, 80, 110, 29, 1);
        /*  dudaskank suggest replacing 999 with SEL_ALL_ENEMIES  */
        special_damage_oneall_enemies(fighter_index, 60, R_EARTH, SEL_ALL_ENEMIES, 1);
        fighter[fighter_index].atrack[fighter[fighter_index].csmem] = 3;
        break;
    case 114:
        strcpy(attack_string, _("Stunning Strike"));
        display_attack_string = 1;
        tempa.fighterStats[A_ATT] = tempa.fighterStats[A_ATT] * 8 / 10;
        kqCombat.fight(fighter_index, tgt, 1);
        display_attack_string = false;
        if (non_dmg_save(tgt, 80) == 0 && kqCombat.ta[tgt] != MISS)
        {
            fighter[tgt].fighterSpellEffectStats[S_STOP] = 2;
        }
        fighter[fighter_index].atrack[fighter[fighter_index].csmem] = 4;
        break;
    case 115:
        strcpy(attack_string, _("Howl"));
        draw_spellsprite(0, 1, 14, 0);
        affected_targets = 0;
        for (target_fighter_index = 0; target_fighter_index < numchrs; target_fighter_index++)
        {
            if (fighter[fighter_index].fighterSpellEffectStats[S_MUTE] == 0)
            {
                if (res_throw(target_fighter_index, S_CHARM) == 0 && non_dmg_save(target_fighter_index, 65) == 0 && fighter[target_fighter_index].fighterSpellEffectStats[S_STONE] == 0)
                {
                    fighter[target_fighter_index].fighterSpellEffectStats[S_CHARM] = kqrandom->random_range_exclusive(2, 4);
                    kqCombat.ta[target_fighter_index] = NODISPLAY;
                }
                else
                {
                    kqCombat.ta[target_fighter_index] = MISS;
                    affected_targets++;
                }
            }
            else
            {
                kqCombat.ta[target_fighter_index] = MISS;
                affected_targets++;
            }
        }
        if (affected_targets > 0)
        {
            display_amount(0, eFont::FONT_WHITE, 1);
        }
        fighter[fighter_index].atrack[fighter[fighter_index].csmem] = 3;
        break;
    case 116:
        strcpy(attack_string, _("Rasp"));
        draw_spellsprite(0, 1, 48, 0);
        for (target_fighter_index = 0; target_fighter_index < numchrs; target_fighter_index++)
        {
            b = fighter[target_fighter_index].fighterHealth / 3;
            kqCombat.ta[target_fighter_index] = 0 - b;
        }
        display_amount(0, eFont::FONT_WHITE, 1);
        for (target_fighter_index = 0; target_fighter_index < numchrs; target_fighter_index++)
        {
            adjust_hp(target_fighter_index, kqCombat.ta[target_fighter_index]);
        }
        for (target_fighter_index = 0; target_fighter_index < numchrs; target_fighter_index++)
        {
            b = fighter[target_fighter_index].fighterMagic / 3;
            kqCombat.ta[target_fighter_index] = 0 - b;
        }
        display_amount(0, eFont::FONT_RED, 1);
        for (target_fighter_index = 0; target_fighter_index < numchrs; target_fighter_index++)
        {
            adjust_mp(target_fighter_index, kqCombat.ta[target_fighter_index]);
        }
        fighter[fighter_index].atrack[fighter[fighter_index].csmem] = 3;
        break;
    case 117:
        strcpy(attack_string, _("Shadow Blast"));
        draw_spellsprite(0, 1, 49, 1);
        special_damage_oneall_enemies(fighter_index, 75, R_BLACK, SEL_ALL_ENEMIES, 1);
        fighter[fighter_index].atrack[fighter[fighter_index].csmem] = 3;
    default:
        break;
    }
}
