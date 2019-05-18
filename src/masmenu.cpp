#include <stdio.h>
#include <string.h>

#include "combat.h"
#include "draw.h"
#include "enums.h"
#include "gfx.h"
#include "input.h"
#include "kq.h"
#include "magic.h"
#include "masmenu.h"
#include "menu.h"
#include "res.h"
#include "selector.h"
#include "setup.h"
/*! \file
 * \brief Menus for spells
 *
 * Menu functions for grimoire (spell book)
 * and functions for learning new spells.
 * \author JB
 * \date ????????
 */

/*  Global variables  */
int close_menu;

/*  Internal functions  */
static int need_spell(size_t, size_t);
static void camp_draw_spell_menu(size_t, size_t, size_t);
static void camp_spell_targeting(size_t, size_t);
static int camp_castable(int, int);

/*! \brief Check if spell is castable
 *
 * Perform the necessary checking to determine if a spell can be
 * cast while camping and if the mp exists to do so.
 *
 * \param   who Index of caster
 * \param   sno Spell number
 * \returns 1 if spell was cast, 0 otherwise
 */
static int camp_castable(int who, int sno)
{
    if (sno == M_VISION || (sno == M_WARP && g_map.can_warp == 0))
    {
        return 0;
    }
    if (magic[sno].use == USE_ANY_INF || magic[sno].use == USE_CAMP_INF)
    {
        if (party[activeAvatarIds[who]].mp >= mp_needed(who, sno))
        {
            return 1;
        }
    }
    return 0;
}

/*! \brief Draw spell menu
 *
 * Draw the current character's grimoire contents.
 *
 * \param   caster_fighter_index Index of caster
 * \param   spell_page Page that spell is found on
 * \param   spell_page_cursor Cursor on current page
 */
static void camp_draw_spell_menu(size_t caster_fighter_index, size_t spell_page, size_t spell_page_cursor)
{
    eFontColor text_color;
    size_t spell_index, pidx_index, current_spell, first_spell_index;

    pidx_index = activeAvatarIds[caster_fighter_index];
    first_spell_index = party[pidx_index].spells[spell_page * NUM_SPELLS_PER_PAGE + spell_page_cursor];
    kqDraw.menubox(double_buffer, 80 + xofs, 12 + yofs, 18, 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
    kqDraw.print_font(double_buffer, 140 + xofs, 20 + yofs, _("Magic"), eFontColor::FONTCOLOR_GOLD);
    kqDraw.menubox(double_buffer, 80 + xofs, 36 + yofs, 18, 5, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
    draw_playerstat(double_buffer, pidx_index, 88 + xofs, 44 + yofs);
    kqDraw.menubox(double_buffer, 80 + xofs, 92 + yofs, 18, 12, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
    for (current_spell = 0; current_spell < NUM_SPELLS_PER_PAGE; current_spell++)
    {
        spell_index = party[pidx_index].spells[spell_page * NUM_SPELLS_PER_PAGE + current_spell];
        text_color = eFontColor::FONTCOLOR_DARK;
        if (camp_castable(caster_fighter_index, spell_index) == 1)
        {
            text_color = eFontColor::FONTCOLOR_NORMAL;
        }
        if (spell_index > 0)
        {
            kqDraw.draw_icon(double_buffer, magic[spell_index].icon, 96 + xofs, current_spell * 8 + 100 + yofs);
            kqDraw.print_font(double_buffer, 104 + xofs, current_spell * 8 + 100 + yofs, magic[spell_index].spellName, text_color);
            sprintf(strbuf, "%d", mp_needed(caster_fighter_index, spell_index));
            kqDraw.print_font(double_buffer, 232 - (strlen(strbuf) * 8) + xofs, current_spell * 8 + 100 + yofs, strbuf, text_color);
        }
    }
    kqDraw.menubox(double_buffer, 40 + xofs, 204 + yofs, 28, 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
    kqDraw.print_font(double_buffer, (160 - (strlen(magic[first_spell_index].desc) * 4)) + xofs, 212 + yofs, magic[first_spell_index].desc, eFontColor::FONTCOLOR_NORMAL);
    draw_sprite(double_buffer, pgb[spell_page], 230 + xofs, 194 + yofs);
}

/*! \brief Spell menu
 *
 * Display the magic spell menu and allow the player to
 * arrange or cast spells.
 *
 * \param   c Index of caster
 */
void camp_spell_menu(int c)
{
    int a, b = 0, smove = 0, stop = 0, tsn;
    int pg[2] = {0, 0};
    int ptr[2] = {0, 0};

    if (party[activeAvatarIds[c]].sts[S_MUTE] > 0)
    {
        play_effect(SND_BAD, 128);
        return;
    }
    update_equipstats();
    play_effect(SND_MENU, 128);
    while (!stop)
    {
        Game.do_check_animation();
        kqDraw.drawmap();
        camp_draw_spell_menu(c, pg[smove], ptr[smove]);
        if (smove == 0)
        {
            draw_sprite(double_buffer, mptr, 88 + xofs, ptr[0] * 8 + 100 + yofs);
        }
        else
        {
            if (pg[0] == pg[1])
            {
                draw_sprite(double_buffer, mptr, 88 + xofs, ptr[0] * 8 + 100 + yofs);
            }
            draw_sprite(double_buffer, sptr, 88 + xofs, ptr[1] * 8 + 100 + yofs);
        }
        kqDraw.blit2screen(xofs, yofs);
        PlayerInput.readcontrols();
        if (PlayerInput.down)
        {
            Game.unpress();
            ptr[smove]++;
            if (ptr[smove] > 11)
            {
                ptr[smove] = 0;
            }
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.up)
        {
            Game.unpress();
            ptr[smove]--;
            if (ptr[smove] < 0)
            {
                ptr[smove] = 11;
            }
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.right)
        {
            Game.unpress();
            pg[smove]++;
            if (pg[smove] > 4)
            {
                pg[smove] = 0;
            }
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.left)
        {
            Game.unpress();
            pg[smove]--;
            if (pg[smove] < 0)
            {
                pg[smove] = 4;
            }
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.balt)
        {
            Game.unpress();
            if (smove == 1)
            {
                smove = 0;
                if (pg[0] != pg[1] || ptr[0] != ptr[1])
                {
                    a = pg[0] * NUM_SPELLS_PER_PAGE + ptr[0];
                    b = pg[1] * NUM_SPELLS_PER_PAGE + ptr[1];
                    tsn = party[activeAvatarIds[c]].spells[a];
                    party[activeAvatarIds[c]].spells[a] = party[activeAvatarIds[c]].spells[b];
                    party[activeAvatarIds[c]].spells[b] = tsn;
                }
                if (pg[0] == pg[1] && ptr[0] == ptr[1])
                {
                    a = pg[0] * NUM_SPELLS_PER_PAGE + ptr[0];
                    tsn = party[activeAvatarIds[c]].spells[a];
                    if (tsn > 0)
                    {
                        if (camp_castable(c, tsn) == 1)
                        {
                            camp_spell_targeting(c, tsn);
                        }
                    }
                    smove = 0;
                }
                pg[0] = pg[1];
                ptr[0] = ptr[1];
            }
            else
            {
                if (party[activeAvatarIds[c]].spells[pg[0] * NUM_SPELLS_PER_PAGE + ptr[0]] > 0)
                {
                    smove = 1;
                    pg[1] = pg[0];
                    ptr[1] = ptr[0];
                }
            }
        }
        if (PlayerInput.bctrl)
        {
            Game.unpress();
            if (smove == 1)
            {
                smove = 0;
            }
            else
            {
                stop = 1;
            }
        }
        if (close_menu == 1)
        {
            stop = 1;
        }
    }
}

/*! \brief Select the target(s)
 *
 * Select any necessary targets and prepare the spell.
 *
 * \param   caster_fighter_index Index of spell caster
 * \param   spell_number Spell number
 */
static void camp_spell_targeting(size_t caster_fighter_index, size_t spell_number)
{
    int tg = 0;
    size_t fighter_index;

    if (magic[spell_number].tgt == TGT_NONE || magic[spell_number].tgt > TGT_ALLY_ALL)
    {
        return;
    }
    while (tg != PIDX_UNDEFINED)
    {
        if (party[activeAvatarIds[caster_fighter_index]].mp < mp_needed(caster_fighter_index, spell_number))
        {
            return;
        }
        if (magic[spell_number].use != USE_ANY_INF && magic[spell_number].use != USE_CAMP_INF)
        {
            return;
        }
        if (spell_number != M_WARP && spell_number != M_REPULSE)
        {
            tg = select_any_player((eTarget)magic[spell_number].tgt, magic[spell_number].icon, magic[spell_number].spellName);
            if (tg == PIDX_UNDEFINED)
            {
                return;
            }
            if (need_spell(tg, spell_number) == 0)
            {
                play_effect(SND_BAD, 128);
                return;
            }
            fighter[caster_fighter_index].ctmem = tg;
        }
        else
        {
            tg = SEL_ALL_ALLIES;
        }
        fighter[caster_fighter_index].csmem = spell_number;
        for (fighter_index = 0; fighter_index < numchrs; fighter_index++)
        {
            kqCombat.ta[fighter_index] = 0;
        }
        if (cast_spell(caster_fighter_index, 0) == 1)
        {
            if (spell_number == M_WARP || spell_number == M_REPULSE)
            {
                close_menu = 1;
                return;
            }
            else
            {
                /*  DS: This piece of code needs to be here because if a spell
                 * cast fails, you can't adjust your hp.
                 */
                for (fighter_index = 0; fighter_index < numchrs; fighter_index++)
                {
                    adjust_hp(fighter_index, kqCombat.ta[fighter_index]);
                }
            }
            play_effect(SND_TWINKLE, 128);
        }
        else
        {
            play_effect(SND_TWINKLE, 128); /* this should be a failure sound */
        }
        revert_equipstats();
        Game.kq_yield();
    }
}

/*! \brief Character learned new spells?
 *
 * This function is called by level_up() and checks to see
 * if the character has learned any new spells (that s/he did
 * not already know).
 *
 * \param   who Character's index
 * \returns 0 if no spell learned, else spell(s) learned
 */
int learn_new_spells(int who)
{
    int a, p, i, nog, g = 0;

    fullblit(double_buffer, back);
    for (a = 1; a < NUM_SPELLS; a++)
    {
        nog = 1;
        for (i = 0; i < 60; i++)
        {
            if (party[who].spells[i] == a)
            {
                nog = 0;
            }
        }
        if (magic[a].clvl[who] == 0 || magic[a].clvl[who] > party[who].lvl)
        {
            nog = 0;
        }
        if (nog == 1)
        {
            p = 60;
            for (i = 60; i > 0; i--)
            {
                if (party[who].spells[i - 1] == 0)
                {
                    p = i - 1;
                }
            }
            if (p < 60)
            {
                if (in_combat == 1)
                {
                    sprintf(strbuf, _("%s learned %s"), party[who].playerName.c_str(), magic[a].spellName);
                    fullblit(back, double_buffer);
                    kqDraw.menubox(double_buffer, 148 - (strlen(strbuf) * 4), 152, strlen(strbuf) + 1, 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
                    kqDraw.draw_icon(double_buffer, magic[a].icon, 156 - (strlen(strbuf) * 4), 160);
                    kqDraw.print_font(double_buffer, 164 - (strlen(strbuf) * 4), 160, strbuf, eFontColor::FONTCOLOR_NORMAL);
                    kqDraw.blit2screen(0, 0);
                    Game.wait_enter();
                    g++;
                }
                party[who].spells[p] = a;
            }
        }
    }
    fullblit(back, double_buffer);
    return g;
}

/*! \brief Does target need spell?
 *
 * Does the target even need the spell that's been selected?
 *
 * \param   target_fighter_index Target
 * \param   spell_number Index of spell
 * \returns 0 if spell failed, 1 if success
 */
static int need_spell(size_t target_fighter_index, size_t spell_number)
{
    size_t stats_index;
    size_t figher_index, victim_figher_index = 0;
    uint32_t affected_targets;

    if (target_fighter_index < numchrs)
    {
        victim_figher_index = activeAvatarIds[target_fighter_index];
    }
    switch (spell_number)
    {
    case M_RESTORE:
        if (party[victim_figher_index].sts[S_POISON] == 0 && party[victim_figher_index].sts[S_BLIND] == 0)
        {
            return 0;
        }
        break;
    case M_RECOVERY:
        affected_targets = 0;
        for (stats_index = 0; stats_index <= S_SLEEP; stats_index++)
        {
            if (party[victim_figher_index].sts[stats_index] != 0)
            {
                affected_targets++;
            }
        }
        if (affected_targets == 0 || party[victim_figher_index].sts[S_DEAD] != 0)
        {
            return 0;
        }
        break;
    case M_LIFE:
    case M_FULLLIFE:
        if (party[victim_figher_index].sts[S_DEAD] == 0)
        {
            return 0;
        }
        break;
    case M_CURE1:
    case M_CURE2:
    case M_CURE3:
    case M_CURE4:
        if (target_fighter_index == SEL_ALL_ALLIES)
        {
            affected_targets = 0;
            for (figher_index = 0; figher_index < numchrs; figher_index++)
            {
                if (party[activeAvatarIds[figher_index]].hp == party[activeAvatarIds[figher_index]].mhp || party[activeAvatarIds[figher_index]].sts[S_STONE] != 0 || party[activeAvatarIds[figher_index]].sts[S_DEAD] != 0)
                {
                    affected_targets++;
                }
            }
            if (affected_targets == numchrs)
            {
                return 0;
            }
        }
        else
        {
            if (party[victim_figher_index].hp == party[victim_figher_index].mhp)
            {
                return 0;
            }
        }
        break;
    case M_WARP:

        /* RB FIXME What was this supposed to do? */
        /* TT: This means that if you're on a map where you can warp from (main.map)
         *     you can get away from this battle.  But if you're somewhere that the
         *     map is defined as 'can_warp = 0', you can't use the warp spell there.
         */
        if (g_map.can_warp == 0)
        {
            return 0;
        }
        break;
    case M_REPULSE:
        return 1;
    default:
        return 0;
    }
    return 1;
}
