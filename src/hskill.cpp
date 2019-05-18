/*! \file
 * \brief Hero skills
 *
 * \author JB
 * \date ????????
 */

#include <memory>
#include <stdio.h>
#include <string.h>

#include "combat.h"
#include "constants.h"
#include "draw.h"
#include "effects.h"
#include "gfx.h"
#include "heroc.h"
#include "hskill.h"
#include "itemmenu.h"
#include "kq.h"
#include "magic.h"
#include "random.h"
#include "res.h"
#include "selector.h"
#include "setup.h"
#include "timing.h"

/* Internal function */
static void infusion(int, int);
void reveal(int);

/*! \brief Is hero's special skill available?
 *
 * Check if the hero can use his/her special
 * skill at this point in combat.
 *
 * \param   fighter_index Hero to check (index into activeAvatarIds[])
 * \returns 1 if skill is available, 0 otherwise
 */
int hero_skillcheck(size_t fighter_index)
{
    size_t weapon_index = 0;
    size_t pidx_index = activeAvatarIds[fighter_index];
    size_t target_fighter_index;
    uint32_t can_be_affected = 0;

    switch (pidx_index)
    {
    case SENSAR:
        if (fighter[fighter_index].fighterHealth <= fighter[fighter_index].fighterMaxHealth / 10)
        {
            return 0;
        }
        else
        {
            return 1;
        }
        break;

    case SARINA:
        weapon_index = items[party[pidx_index].eqp[0]].icon;
        if (weapon_index != W_SWORD && weapon_index != W_AXE && weapon_index != W_KNIFE && weapon_index != W_CHENDIGAL)
        {
            return 0;
        }
        // See whether any enemies CAN be turned to stone.
        for (target_fighter_index = MAX_PARTY_SIZE; target_fighter_index < MAX_PARTY_SIZE + kqCombat.num_enemies; target_fighter_index++)
        {
            if (fighter[target_fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD && fighter[target_fighter_index].fighterSpellEffectStats[S_STONE] == 0)
            {
                can_be_affected++;
            }
        }
        if (can_be_affected > 1)
        {
            return 1;
        }
        else
        {
            return 0;
        }
        break;

    case CORIN:
        if (fighter[fighter_index].fighterSpellEffectStats[S_MUTE] > 0)
        {
            return 0;
        }
        if (fighter[fighter_index].aux == 0)
        {
            fighter[fighter_index].aux = 2;
            can_be_affected = heroc.available_spells(fighter_index);
            fighter[fighter_index].aux = 0;
            if (can_be_affected > 0)
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }
        else
        {
            return 0;
        }
    case AJATHAR:
        if (fighter[fighter_index].fighterSpellEffectStats[S_MUTE] > 0)
        {
            return 0;
        }
        for (target_fighter_index = MAX_PARTY_SIZE; target_fighter_index < MAX_PARTY_SIZE + kqCombat.num_enemies; target_fighter_index++)
        {
            if (fighter[target_fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD && fighter[target_fighter_index].fighterSpellEffectStats[S_STONE] == 0 && fighter[target_fighter_index].unl > 0)
            {
                can_be_affected++;
            }
        }
        if (can_be_affected > 0)
        {
            fighter[fighter_index].unl = 1;
        }
        else
        {
            fighter[fighter_index].unl = 0;
        }
        return 1;
        break;

    case CASANDRA:
        if (fighter[fighter_index].fighterSpellEffectStats[S_MUTE] > 0)
        {
            return 0;
        }
        if (fighter[fighter_index].aux == 0)
        {
            fighter[fighter_index].atrack[2] = fighter[fighter_index].mrp;
            fighter[fighter_index].mrp = fighter[fighter_index].mrp * 15 / 10;
            can_be_affected = heroc.available_spells(fighter_index);
            fighter[fighter_index].mrp = fighter[fighter_index].atrack[2];
            if (can_be_affected > 0)
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }
        else
        {
            return 0;
        }
        break;

    case TEMMIN:
        if (numchrs == 1)
        {
            return 0;
        }
        for (target_fighter_index = 0; target_fighter_index < numchrs; target_fighter_index++)
        {
            if (fighter[target_fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD)
            {
                can_be_affected++;
            }
        }
        if (can_be_affected > 1)
        {
            return 1;
        }
        else
        {
            return 0;
        }
        break;

    case AYLA:
    case NOSLOM:
        return 1;
        break;
    }
    return 0;
}

/*! \brief Do infusion skill
 *
 * This function is only used for Corin when he
 * uses his Infuse ability.
 *
 * \param   c Hero (index in fighter[] array)
 * \param   sn Thing to infuse
 */
static void infusion(int c, int sn)
{
    size_t j;

    switch (sn)
    {

    /* TT TODO: Sort all of these by element type
     * (poison, fire, etc) then by damage.
     */

    /* Increase resistance to Earthquake attacks */
    case M_TREMOR:
        fighter[c].fighterResistance[R_EARTH] += 5;
        fighter[c].fighterStats[A_DEF] += 15;
        fighter[c].fighterStats[A_MAG] += 10;
        fighter[c].welem = 0;
        break;

    case M_EARTHQUAKE:
        fighter[c].fighterResistance[R_EARTH] += 10;
        fighter[c].fighterStats[A_DEF] += 30;
        fighter[c].fighterStats[A_MAG] += 20;
        fighter[c].welem = 0;
        break;

    /* Increase resistance to Dark attacks */
    /* Decrease resistance to Light attacks */
    case M_GLOOM:
        fighter[c].fighterResistance[R_BLACK] += 8;
        fighter[c].fighterResistance[R_WHITE] -= 4;
        fighter[c].fighterStats[A_AUR] += 20;
        fighter[c].welem = 1;
        break;

    case M_NEGATIS:
        fighter[c].fighterResistance[R_BLACK] += 16;
        fighter[c].fighterResistance[R_WHITE] -= 8;
        fighter[c].fighterStats[A_AUR] += 40;
        fighter[c].welem = 1;
        break;

    /* Increase resistance to Fire attacks */
    /* Decrease resistance to Water & Ice attacks */
    case M_SCORCH:
        fighter[c].fighterResistance[R_FIRE] += 4;
        fighter[c].fighterResistance[R_WATER]--;
        fighter[c].fighterResistance[R_ICE]--;
        fighter[c].fighterStats[A_ATT] += 10;
        fighter[c].fighterStats[A_HIT] += 10;
        fighter[c].welem = 2;
        break;

    case M_FIREBLAST:
        fighter[c].fighterResistance[R_FIRE] += 8;
        fighter[c].fighterResistance[R_WATER] -= 2;
        fighter[c].fighterResistance[R_ICE] -= 2;
        fighter[c].fighterStats[A_ATT] += 20;
        fighter[c].fighterStats[A_HIT] += 20;
        fighter[c].welem = 2;
        break;

    case M_FLAMEWALL:
        fighter[c].fighterResistance[R_FIRE] += 12;
        fighter[c].fighterResistance[R_WATER] -= 4;
        fighter[c].fighterResistance[R_ICE] -= 4;
        fighter[c].fighterStats[A_ATT] += 40;
        fighter[c].fighterStats[A_HIT] += 40;
        fighter[c].welem = 2;
        break;

    /* Increase resistance to Thunder attacks */
    case M_SHOCK:
        fighter[c].fighterResistance[R_THUNDER] += 3;
        fighter[c].fighterStats[A_EVD] += 10;
        fighter[c].welem = 3;
        break;

    case M_LIGHTNING:
        fighter[c].fighterResistance[R_THUNDER] += 6;
        fighter[c].fighterStats[A_EVD] += 25;
        fighter[c].welem = 3;
        break;

    case M_THUNDERSTORM:
        fighter[c].fighterResistance[R_THUNDER] += 12;
        fighter[c].fighterStats[A_EVD] += 50;
        fighter[c].welem = 3;
        break;

    /* Increase resistance to Air attacks */
    case M_WHIRLWIND:
        fighter[c].fighterResistance[R_AIR] += 5;
        fighter[c].fighterStats[A_EVD] += 15;
        fighter[c].fighterStats[A_SPD] += 10;
        fighter[c].welem = 4;
        break;

    case M_TORNADO:
        fighter[c].fighterResistance[R_AIR] += 10;
        fighter[c].fighterStats[A_EVD] += 30;
        fighter[c].fighterStats[A_SPD] += 20;
        fighter[c].welem = 4;
        break;

    /* Increase resistance to Light attacks */
    /* Decrease resistance to Dark attacks */
    case M_FADE:
        fighter[c].fighterResistance[R_WHITE] += 5;
        fighter[c].fighterResistance[R_BLACK] -= 2;
        fighter[c].fighterStats[A_SPI] += 10;
        fighter[c].welem = 5;
        break;

    case M_LUMINE:
        fighter[c].fighterResistance[R_WHITE] += 10;
        fighter[c].fighterResistance[R_BLACK] -= 5;
        fighter[c].fighterStats[A_SPI] += 25;
        fighter[c].welem = 5;
        break;

    /* Increase resistance to Water attacks */
    /* Decrease resistance to Thunder attacks */
    case M_FLOOD:
        fighter[c].fighterResistance[R_WATER] += 5;
        fighter[c].fighterResistance[R_THUNDER] -= 5;
        for (j = 9; j < R_TOTAL_RES; j++)
        {
            fighter[c].fighterResistance[j] += 3;
            if (fighter[c].fighterResistance[j] > 10)
            {
                fighter[c].fighterResistance[j] = 10;
            }
        }
        fighter[c].welem = 6;
        break;

    case M_TSUNAMI:
        fighter[c].fighterResistance[R_WATER] += 10;
        fighter[c].fighterResistance[R_THUNDER] -= 10;
        for (j = 9; j < R_TOTAL_RES; j++)
        {
            fighter[c].fighterResistance[j] += 6;
            if (fighter[c].fighterResistance[j] > 10)
            {
                fighter[c].fighterResistance[j] = 10;
            }
        }
        fighter[c].welem = 6;
        break;

    /* Increase resistance to Ice & Water attacks */
    /* Decrease resistance to Fire attacks */
    case M_FROST:
        fighter[c].fighterResistance[R_ICE] += 7;
        fighter[c].fighterResistance[R_WATER] += 4;
        fighter[c].fighterResistance[R_FIRE] -= 5;
        fighter[c].fighterStats[A_DEF] += 10;
        fighter[c].welem = 7;
        break;

    case M_BLIZZARD:
        fighter[c].fighterResistance[R_ICE] += 14;
        fighter[c].fighterResistance[R_WATER] += 8;
        fighter[c].fighterResistance[R_FIRE] -= 10;
        fighter[c].fighterStats[A_DEF] += 25;
        fighter[c].welem = 7;
        break;

    /* Increase resistance to Poison attacks */
    case M_VENOM:
        fighter[c].fighterResistance[R_POISON] += 4;
        j = fighter[c].fighterMaxHealth / 10;
        if (j < 10)
        {
            j = 10;
        }
        fighter[c].fighterHealth += j;
        fighter[c].fighterMaxHealth += j;
        fighter[c].welem = 8;
        break;

    case M_VIRUS:
        fighter[c].fighterResistance[R_POISON] += 8;
        j = fighter[c].fighterMaxHealth * 25 / 100;
        if (j < 40)
        {
            j = 40;
        }
        fighter[c].fighterHealth += j;
        fighter[c].fighterMaxHealth += j;
        fighter[c].welem = 8;
        break;

    case M_PLAGUE:
        fighter[c].fighterResistance[R_POISON] += 12;
        j = fighter[c].fighterMaxHealth * 4 / 10;
        if (j < 80)
        {
            j = 80;
        }
        fighter[c].fighterHealth += j;
        fighter[c].fighterMaxHealth += j;
        fighter[c].welem = 8;
        break;
    }

    for (j = 0; j < 9; j++)
    {
        if (fighter[c].fighterResistance[j] < -10)
        {
            fighter[c].fighterResistance[j] = -10;
        }
        if (fighter[c].fighterResistance[j] > 20)
        {
            fighter[c].fighterResistance[j] = 20;
        }
    }
}

void reveal(int tgt)
{
    uint32_t c, g = 0, b;
    uint32_t d = 0;
    int draw_x, draw_y;
    char resistance;

    kqDraw.do_transition(TRANS_FADE_OUT, 4);
    kqDraw.menubox(double_buffer, 84, 56, 17, 13, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
    sprintf(strbuf, _("Name: %s"), fighter[tgt].fighterName.c_str());
    kqDraw.print_font(double_buffer, 92, 64, strbuf, eFontColor::FONTCOLOR_NORMAL);
    sprintf(strbuf, _("Level: %d"), fighter[tgt].fighterLevel);
    kqDraw.print_font(double_buffer, 92, 72, strbuf, eFontColor::FONTCOLOR_NORMAL);
    sprintf(strbuf, _("HP: %d/%d"), fighter[tgt].fighterHealth, fighter[tgt].fighterMaxHealth);
    kqDraw.print_font(double_buffer, 92, 80, strbuf, eFontColor::FONTCOLOR_NORMAL);
    sprintf(strbuf, _("MP: %d/%d"), fighter[tgt].fighterMagic, fighter[tgt].fighterMaxMagic);
    kqDraw.print_font(double_buffer, 92, 88, strbuf, eFontColor::FONTCOLOR_NORMAL);
    kqDraw.print_font(double_buffer, 92, 96, _("Earth"), eFontColor::FONTCOLOR_NORMAL);
    kqDraw.print_font(double_buffer, 92, 104, _("Black"), eFontColor::FONTCOLOR_NORMAL);
    kqDraw.print_font(double_buffer, 92, 112, _("Fire"), eFontColor::FONTCOLOR_NORMAL);
    kqDraw.print_font(double_buffer, 92, 120, _("Thunder"), eFontColor::FONTCOLOR_NORMAL);
    kqDraw.print_font(double_buffer, 92, 128, _("Air"), eFontColor::FONTCOLOR_NORMAL);
    kqDraw.print_font(double_buffer, 92, 136, _("White"), eFontColor::FONTCOLOR_NORMAL);
    kqDraw.print_font(double_buffer, 92, 144, _("Water"), eFontColor::FONTCOLOR_NORMAL);
    kqDraw.print_font(double_buffer, 92, 152, _("Ice"), eFontColor::FONTCOLOR_NORMAL);
    for (c = 0; c < 8; c++)
    {
        draw_x = 156;
        draw_y = c * 8 + 97;
        rectfill(double_buffer, draw_x, draw_y, draw_x + 70, draw_y + 6, 3);

        resistance = fighter[tgt].fighterResistance[c];
        if (resistance < 0)
        {
            // 18: bright red, meaning WEAK defense
            g = 18;
            d = abs(resistance);
        }
        else if (resistance >= 0 && resistance <= 10)
        {
            // 34: bright green, meaning so-so defense
            g = 34;
            d = resistance;
        }
        else if (resistance > 10)
        {
            // 50: bright blue, meaning STRONG defense
            g = 50;
            d = resistance - 10;
        }

        if (d > 0)
        {
            for (b = 0; b < d; b++)
            {
                rectfill(double_buffer, b * 7 + 157, c * 8 + 98, b * 7 + 162, c * 8 + 102, g + b);
            }
        }
    }
    kqDraw.blit2screen(0, 0);
    kqDraw.do_transition(TRANS_FADE_IN, 4);
    Game.wait_enter();
}

/*! \brief Activate the special skill
 *
 * This function activates the special skill for a hero,
 * including targeting etc. if required.
 *
 * \param   attack_fighter_index Hero to process
 * \returns 1 if the skill was used, otherwise 0
 */
int skill_use(size_t attack_fighter_index)
{
    int tgt, found_item, a, b, c, p, cts, tx, ty, next_target = 0, nn[NUM_FIGHTERS];
    size_t enemy_index;
    size_t fighter_index;
    std::unique_ptr<Raster> temp;
    tempa = status_adjust(attack_fighter_index);
    switch (activeAvatarIds[attack_fighter_index])
    {
    case SENSAR:
        tgt = select_enemy(attack_fighter_index, TGT_ENEMY_ONE);
        if (tgt == -1)
        {
            return 0;
        }
        enemy_index = (unsigned int)tgt;
        temp = std::unique_ptr<Raster>(new Raster(320, 240));
        blit(kqCombat.backart, temp.get(), 0, 0, 0, 0, 320, 240);
        kqDraw.color_scale(temp.get(), kqCombat.backart, 16, 31);
        b = fighter[attack_fighter_index].fighterMaxHealth / 20;
        strcpy(attack_string, _("Rage"));
        display_attack_string = true;
        tempa.fighterStats[A_ATT] = fighter[attack_fighter_index].fighterStats[A_ATT];
        tempa.fighterStats[A_HIT] = fighter[attack_fighter_index].fighterStats[A_HIT];
        if (fighter[enemy_index].fighterCanCriticalHit == 1)
        {
            tempa.fighterStats[A_ATT] += b;
            tempa.fighterStats[A_HIT] += b;
        }
        kqCombat.fight(attack_fighter_index, enemy_index, 1);
        if (fighter[enemy_index].fighterSpellEffectStats[S_DEAD] == eDeathType::IS_DEAD)
        {
            for (fighter_index = MAX_PARTY_SIZE; fighter_index < MAX_PARTY_SIZE + kqCombat.num_enemies; fighter_index++)
            {
                if (fighter[fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD)
                {
                    nn[next_target] = fighter_index;
                    next_target++;
                }
            }
            if (next_target > 0)
            {
                enemy_index = nn[kqrandom->random_range_exclusive(0, next_target)];
                kqCombat.fight(attack_fighter_index, enemy_index, 1);
            }
        }

        fighter[attack_fighter_index].fighterHealth -= (b * 2);
        kqCombat.ta[attack_fighter_index] = (b * 2);
        display_attack_string = false;
        blit(temp.get(), kqCombat.backart, 0, 0, 0, 0, 320, 240);
        display_amount(attack_fighter_index, eFont::FONT_DECIDE, 0);
        if (fighter[attack_fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD && fighter[attack_fighter_index].fighterHealth <= 0)
        {
            kqCombat.fkill(attack_fighter_index);
            death_animation(attack_fighter_index, 0);
        }
        break;

    case SARINA:
        fighter[attack_fighter_index].ctmem = 1000;
        strcpy(attack_string, _("Sweep"));
        display_attack_string = true;
        tempa.fighterStats[A_ATT] = tempa.fighterStats[A_ATT] * 75 / 100;
        fighter[attack_fighter_index].fighterAttackSpriteFrame = 6;
        kqCombat.fighterImageDatafileX = -1;
        kqCombat.fighterImageDatafileY = -1;
        kqCombat.battle_render(0, 0, 0);
        kqDraw.blit2screen(0, 0);
        kq_wait(150);
        kqCombat.multi_fight(attack_fighter_index);
        display_attack_string = false;
        break;

    case CORIN:
        strcpy(attack_string, _("Elemental Infusion"));
        display_attack_string = true;
        fighter[attack_fighter_index].aux = 2;
        if (heroc.combat_spell_menu(attack_fighter_index) == 1)
        {
            draw_castersprite(attack_fighter_index, eff[magic[fighter[attack_fighter_index].csmem].eff].kolor);
            kqCombat.fighterImageDatafileX = -1;
            kqCombat.fighterImageDatafileY = -1;
            play_effect(22, 128);
            kqDraw.convert_cframes(attack_fighter_index,
                eff[magic[fighter[attack_fighter_index].csmem].eff].kolor - 3,
                eff[magic[fighter[attack_fighter_index].csmem].eff].kolor + 3, 0);
            kqCombat.battle_render(0, 0, 0);
            fullblit(double_buffer, back);
            for (p = 0; p < 2; p++)
            {
                for (a = 0; a < 16; a++)
                {
                    tx = fighter[attack_fighter_index].fighterImageDatafileX + (fighter[attack_fighter_index].fighterImageDatafileWidth / 2);
                    ty = fighter[attack_fighter_index].fighterImageDatafileY + (fighter[attack_fighter_index].fighterImageDatafileHeight / 2);
                    if (p == 0)
                    {
                        circlefill(double_buffer, tx, ty, a, eff[magic[fighter[attack_fighter_index].csmem].eff].kolor);
                    }
                    else
                    {
                        circlefill(double_buffer, tx, ty, 15 - a, eff[magic[fighter[attack_fighter_index].csmem].eff].kolor);
                        kqCombat.draw_fighter(attack_fighter_index, 0);
                    }
                    kqDraw.blit2screen(0, 0);
                    kq_wait(50);
                    fullblit(back, double_buffer);
                }
            }
            kqDraw.revert_cframes(attack_fighter_index, 0);
            kqCombat.battle_render(0, 0, 0);
            kqDraw.blit2screen(0, 0);
            infusion(attack_fighter_index, fighter[attack_fighter_index].csmem);
            c = mp_needed(attack_fighter_index, fighter[attack_fighter_index].csmem);
            if (c < 1)
            {
                c = 1;
            }
            fighter[attack_fighter_index].fighterMagic -= c;
            kqCombat.bIsEtherEffectActive[attack_fighter_index] = false;
            fighter[attack_fighter_index].aux = 1;
        }
        else
        {
            fighter[attack_fighter_index].aux = 0;
            display_attack_string = false;
            return 0;
        }
        display_attack_string = false;
        fighter[attack_fighter_index].fighterSpellEffectStats[S_INFUSE] = magic[fighter[attack_fighter_index].csmem].elem;
        break;

    case AJATHAR:
        if (fighter[attack_fighter_index].unl > 0)
        {
            strcpy(attack_string, _("Dispel Undead"));
            display_attack_string = true;
            fullblit(double_buffer, back);
            for (a = 0; a < 14; a++)
            {
                kqDraw.convert_cframes(MAX_PARTY_SIZE, 1 + a, 15, 1);
                for (fighter_index = MAX_PARTY_SIZE; fighter_index < MAX_PARTY_SIZE + kqCombat.num_enemies; fighter_index++)
                {
                    if (is_active(fighter_index))
                    {
                        kqCombat.draw_fighter(fighter_index, 0);
                    }
                }
                kqDraw.blit2screen(0, 0);
                kq_wait(50);
                fullblit(back, double_buffer);
            }
            kqDraw.revert_cframes(MAX_PARTY_SIZE, 1);
            display_attack_string = false;
            b = fighter[attack_fighter_index].fighterLevel * 15;
            for (fighter_index = MAX_PARTY_SIZE; fighter_index < MAX_PARTY_SIZE + kqCombat.num_enemies; fighter_index++)
            {
                if (fighter[fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD && fighter[fighter_index].fighterMaxHealth > 0)
                {
                    if (fighter[fighter_index].unl == 99 ||
                        fighter[fighter_index].unl == 0)
                    {
                        cts = 0;
                    }
                    else
                    {
                        a = (fighter[attack_fighter_index].fighterLevel + 5) - fighter[fighter_index].unl;
                        if (a > 0)
                        {
                            cts = a * 8;
                        }
                        else
                        {
                            cts = 0;
                        }
                    }
                    if (kqrandom->random_range_exclusive(0, 100) < cts)
                    {
                        if (b >= fighter[fighter_index].fighterHealth)
                        {
                            b -= fighter[fighter_index].fighterHealth;
                            kqCombat.deffect[fighter_index] = 1;
                            kqCombat.fkill(fighter_index);
                        }
                    }
                }
            }
            death_animation(MAX_PARTY_SIZE, 1);
            kqCombat.fighterImageDatafileX = -1;
            kqCombat.fighterImageDatafileY = -1;
            kqCombat.battle_render(attack_fighter_index, attack_fighter_index, 0);
        }
        else
        {
            a = kqrandom->random_range_exclusive(0, 100);
            c = fighter[attack_fighter_index].fighterLevel / 10 + 1;
            if (a < 25)
            {
                b = kqrandom->random_range_exclusive(0, 5 * c) + 1;
            }
            else
            {
                if (a < 90)
                {
                    b = kqrandom->random_range_exclusive(0, 10 * c) + (20 * c);
                }
                else
                {
                    b = kqrandom->random_range_exclusive(0, 25 * c) + (50 * c);
                }
            }
            strcpy(attack_string, _("Divine Cure"));
            display_attack_string = true;
            draw_spellsprite(0, 1, 15, 1);
            display_attack_string = false;
            for (fighter_index = 0; fighter_index < numchrs; fighter_index++)
            {
                if (fighter[fighter_index].fighterSpellEffectStats[S_STONE] == 0 &&
                    fighter[fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD)
                {
                    kqCombat.ta[fighter_index] = b;
                    kqCombat.ta[fighter_index] = do_shell_check(fighter_index, kqCombat.ta[fighter_index]);
                }
            }
            display_amount(0, eFont::FONT_YELLOW, 1);
            for (fighter_index = 0; fighter_index < numchrs; fighter_index++)
            {
                if (fighter[fighter_index].fighterSpellEffectStats[S_STONE] == 0 &&
                    fighter[fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD)
                {
                    adjust_hp(fighter_index, kqCombat.ta[fighter_index]);
                }
            }
        }
        break;

    case CASANDRA:
        fighter[attack_fighter_index].atrack[0] = fighter[attack_fighter_index].fighterStats[A_AUR];
        fighter[attack_fighter_index].atrack[1] = fighter[attack_fighter_index].fighterStats[A_SPI];
        fighter[attack_fighter_index].fighterStats[A_AUR] = fighter[attack_fighter_index].fighterStats[A_AUR] * 15 / 10;
        fighter[attack_fighter_index].fighterStats[A_SPI] = fighter[attack_fighter_index].fighterStats[A_SPI] * 15 / 10;
        fighter[attack_fighter_index].atrack[2] = fighter[attack_fighter_index].mrp;
        fighter[attack_fighter_index].mrp = fighter[attack_fighter_index].mrp * 15 / 10;
        if (heroc.combat_spell_menu(attack_fighter_index) == 1)
        {
            kqCombat.bIsEtherEffectActive[attack_fighter_index] = false;
            fighter[attack_fighter_index].aux = 1;
            fighter[attack_fighter_index].fighterStats[A_AUR] = fighter[attack_fighter_index].atrack[0];
            fighter[attack_fighter_index].fighterStats[A_SPI] = fighter[attack_fighter_index].atrack[1];
            fighter[attack_fighter_index].mrp = fighter[attack_fighter_index].atrack[2];
        }
        else
        {
            fighter[attack_fighter_index].fighterStats[A_AUR] = fighter[attack_fighter_index].atrack[0];
            fighter[attack_fighter_index].fighterStats[A_SPI] = fighter[attack_fighter_index].atrack[1];
            fighter[attack_fighter_index].mrp = fighter[attack_fighter_index].atrack[2];
            return 0;
        }
        break;

    case TEMMIN:
        fighter[attack_fighter_index].aux = 1;
        fighter[attack_fighter_index].fighterWillDefend = 1;
        break;

    case AYLA:
        tgt = select_enemy(attack_fighter_index, TGT_ENEMY_ONE);
        if (tgt == -1)
        {
            return 0;
        }
        enemy_index = (uint32_t)tgt;
        tx = fighter[attack_fighter_index].fighterImageDatafileX;
        ty = fighter[attack_fighter_index].fighterImageDatafileY;
        fighter[attack_fighter_index].fighterImageDatafileX = fighter[enemy_index].fighterImageDatafileX - 16;
        fighter[attack_fighter_index].fighterImageDatafileY = fighter[enemy_index].fighterImageDatafileY + fighter[enemy_index].fighterImageDatafileHeight - 40;
        fighter[attack_fighter_index].fighterSpriteFacing = 1;
        strcpy(attack_string, _("Steal"));
        display_attack_string = true;
        kqCombat.battle_render(0, attack_fighter_index + 1, 0);
        kqDraw.blit2screen(0, 0);
        kq_wait(100);
        play_effect(SND_MENU, 128);
        kq_wait(500);
        display_attack_string = false;
        kqCombat.battle_render(attack_fighter_index, attack_fighter_index, 0);
        found_item = 0;
#ifdef DEBUGMODE
        if (debugging > 2)
        {
            if (fighter[enemy_index].fighterStealItemRare > 0)
            {
                /* This steals a rare item from monster, if there is one */
                found_item = fighter[enemy_index].fighterStealItemRare;
                fighter[enemy_index].fighterStealItemRare = 0;
            }
            else if (fighter[enemy_index].fighterStealItemCommon > 0)
            {
                /* This steals a common item from a monster, if there is one */
                found_item = fighter[enemy_index].fighterStealItemCommon;
                fighter[enemy_index].fighterStealItemCommon = 0;
            }
            if (found_item > 0)
            {
                if (check_inventory(found_item, 1) != 0)
                {
                    sprintf(strbuf, _("%s taken!"), items[found_item].itemName);
                    kqDraw.message(strbuf, items[found_item].icon, 0, 0, 0);
                }
            }
            else
            {
                if (fighter[enemy_index].fighterStealItemCommon == 0 &&
                    fighter[enemy_index].fighterStealItemRare == 0)
                {
                    kqDraw.message(_("Nothing to steal!"), 255, 0, 0, 0);
                }
                else
                {
                    kqDraw.message(_("Couldn't steal!"), 255, 0, 0, 0);
                }
            }
        }
#else
        cts = party[activeAvatarIds[attack_fighter_index]].lvl * 2 + 35;
        if (cts > 95)
        {
            cts = 95;
        }
        if (kqrandom->random_range_exclusive(0, 100) < cts)
        {
            if (fighter[enemy_index].fighterStealItemRare > 0 && (kqrandom->random_range_exclusive(0, 100) < 5))
            {
                /* This steals a rare item from monster, if there is one */
                found_item = fighter[enemy_index].fighterStealItemRare;
                fighter[enemy_index].fighterStealItemRare = 0;
            }
            else if (fighter[enemy_index].fighterStealItemCommon > 0 && (kqrandom->random_range_exclusive(0, 100) < 95))
            {
                /* This steals a common item from a monster, if there is one */
                found_item = fighter[enemy_index].fighterStealItemCommon;
                fighter[enemy_index].fighterStealItemCommon = 0;
            }
            if (found_item > 0)
            {
                if (check_inventory(found_item, 1) != 0)
                {
                    sprintf(strbuf, _("%s taken!"), items[found_item].itemName);
                    kqDraw.message(strbuf, items[found_item].icon, 0, 0, 0);
                }
            }
            else
            {
                if (fighter[enemy_index].fighterStealItemCommon == 0 &&
                    fighter[enemy_index].fighterStealItemRare == 0)
                {
                    kqDraw.message(_("Nothing to steal!"), 255, 0, 0, 0);
                }
                else
                {
                    kqDraw.message(_("Couldn't steal!"), 255, 0, 0, 0);
                }
            }
        }
        else
        {
            kqDraw.message(_("Couldn't steal!"), 255, 0, 0, 0);
        }
#endif
        fighter[attack_fighter_index].fighterImageDatafileX = tx;
        fighter[attack_fighter_index].fighterImageDatafileY = ty;
        display_attack_string = false;
        fighter[attack_fighter_index].fighterSpriteFacing = 0;
        kqCombat.battle_render(attack_fighter_index, attack_fighter_index, 0);
        kqDraw.blit2screen(0, 0);
        break;

    case NOSLOM:
        tgt = select_enemy(attack_fighter_index, TGT_ENEMY_ONE);
        if (tgt == -1)
        {
            return 0;
        }
        enemy_index = (uint32_t)tgt;
        reveal(enemy_index);
        break;
    default:
        break;
    }
    return 1;
}
