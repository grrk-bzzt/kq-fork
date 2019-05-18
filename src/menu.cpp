#include <stdio.h>
#include <string.h>

#include "constants.h"
#include "draw.h"
#include "eqpmenu.h"
#include "gfx.h"
#include "heroc.h"
#include "input.h"
#include "itemdefs.h"
#include "itemmenu.h"
#include "kq.h"
#include "masmenu.h"
#include "menu.h"
#include "random.h"
#include "res.h"
#include "selector.h"
#include "setup.h"
#include "structs.h"

/*! \file
 * \brief Main menu functions
 *
 * \author JB
 * \date ??????
 */

namespace KqFork
{
int check_xp(int, int);
void ilist_add(info_list*, const char*, const char*);
void level_up(int);
void quest_info();
void status_screen(size_t);
info_list quest_list;
} // namespace KqFork

/*! \brief Add a new quest item
 *
 * Add a Quest info item to the current set
 * \sa ILIST
 * \param key The title of the item
 * \param text The text to associate with it
 * \author PH
 * \date 20050429
 */
void add_questinfo(const char* key, const char* text)
{
    KqFork::ilist_add(&KqFork::quest_list, key, text);
}

/*! \brief Check for level-ups
 *
 * Checks for level ups.
 *
 * \param   pl - Player
 * \param   ls - Learned new spell
 * \returns 1 if new spell learned, 0 otherwise
 */
int KqFork::check_xp(int pl, int ls)
{
    int stp = 0, z = 0;

    if (party[pl].lvl >= 50)
    {
        return 0;
    }
    while (!stp)
    {
        if (party[pl].xp >= party[pl].next)
        {
            level_up(pl);
            z = 1;
        }
        else
        {
            stp = 1;
        }
        if (party[pl].lvl >= 50)
        {
            stp = 1;
        }
    }
    if (ls == 1)
    {
        learn_new_spells(pl);
    }
    return z;
}

/*! \brief Draws the main menu
 *
 * Draw the menu when the player hits ENTER
 * 20040911 PH Added an extra line in the menu for "Quest Info"
 */
void draw_mainmenu(int swho)
{

    timer_count = 0;
    for (size_t fighter_index = 0; fighter_index < MAX_PARTY_SIZE; fighter_index++)
    {
        eMenuBoxColor menuBoxColor = ((size_t)swho == fighter_index) ?
            eMenuBoxColor::DARKBLUE :
            eMenuBoxColor::SEMI_TRANSPARENT_BLUE;
        kqDraw.menubox(double_buffer, 44 + xofs, fighter_index * 64 + 64 + yofs, 18, 6, menuBoxColor);
    }
    kqDraw.menubox(double_buffer, 204 + xofs, 64 + yofs, 7, 6, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
    kqDraw.menubox(double_buffer, 204 + xofs, 128 + yofs, 7, 6, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
    kqDraw.print_font(double_buffer, 220 + xofs, 72 + yofs, _("Items"), eFontColor::FONTCOLOR_GOLD);
    kqDraw.print_font(double_buffer, 220 + xofs, 80 + yofs, _("Magic"), eFontColor::FONTCOLOR_GOLD);
    kqDraw.print_font(double_buffer, 220 + xofs, 88 + yofs, _("Equip"), eFontColor::FONTCOLOR_GOLD);
    kqDraw.print_font(double_buffer, 220 + xofs, 96 + yofs, _("Spec."), eFontColor::FONTCOLOR_GOLD);
    kqDraw.print_font(double_buffer, 220 + xofs, 104 + yofs, _("Stats"), eFontColor::FONTCOLOR_GOLD);
    kqDraw.print_font(double_buffer, 220 + xofs, 112 + yofs, _("Quest"), eFontColor::FONTCOLOR_GOLD);
    kqDraw.print_font(double_buffer, 212 + xofs, 136 + yofs, _("Time:"), eFontColor::FONTCOLOR_GOLD);
    kqDraw.print_font(double_buffer, 212 + xofs, 164 + yofs, _("Gold:"), eFontColor::FONTCOLOR_GOLD);
    /* PH: print time as h:mm */
    sprintf(strbuf, "%d:%02d", khr, kmin);
    kqDraw.print_font(double_buffer, 268 - (strlen(strbuf) * 8) + xofs, 144 + yofs, strbuf, eFontColor::FONTCOLOR_NORMAL);
    sprintf(strbuf, "%d", gp);
    kqDraw.print_font(double_buffer, 268 - (strlen(strbuf) * 8) + xofs, 172 + yofs, strbuf, eFontColor::FONTCOLOR_NORMAL);
    if (swho != -1)
    {
        kqDraw.menubox(double_buffer, 44 + xofs, swho * 64 + 64 + yofs, 18, 6, eMenuBoxColor::DARKBLUE);
    }
    for (size_t fighter_index = 0; fighter_index < numchrs; fighter_index++)
    {
        draw_playerstat(double_buffer, pidx[fighter_index], 52 + xofs, fighter_index * 64 + 76 + yofs);
    }
}

/*! \brief Draw player's stats
 *
 * Draw the terse stats of a single player.
 * \param   where Bitmap to draw onto
 * \param   player_index_in_party Player (index in party array) to show info for
 * \param   dx left-most x-coord of stats view
 * \param   dy top-most y-coord of stats view
 */
void draw_playerstat(Raster* where, int player_index_in_party, int dx, int dy)
{
    int j;
    players[player_index_in_party].portrait->maskedBlitTo(where, dx, dy);
    kqDraw.print_font(where, dx + 48, dy, party[player_index_in_party].playerName.c_str(), eFontColor::FONTCOLOR_NORMAL);
    kqDraw.draw_stsicon(where, 0, player_index_in_party, 8, dx + 48, dy + 8);
    kqDraw.print_font(where, dx + 48, dy + 16, _("LV"), eFontColor::FONTCOLOR_GOLD);
    sprintf(strbuf, "%d", party[player_index_in_party].lvl);
    kqDraw.print_font(where, dx + 104 - (strlen(strbuf) * 8), dy + 16, strbuf, eFontColor::FONTCOLOR_NORMAL);
    kqDraw.print_font(where, dx + 48, dy + 24, _("HP"), eFontColor::FONTCOLOR_GOLD);
    kqDraw.print_font(where, dx + 104, dy + 24, "/", eFontColor::FONTCOLOR_NORMAL);
    sprintf(strbuf, "%d", party[player_index_in_party].hp);
    j = strlen(strbuf) * 8;
    kqDraw.print_font(where, dx + 104 - j, dy + 24, strbuf, eFontColor::FONTCOLOR_NORMAL);
    sprintf(strbuf, "%d", party[player_index_in_party].mhp);
    j = strlen(strbuf) * 8;
    kqDraw.print_font(where, dx + 144 - j, dy + 24, strbuf, eFontColor::FONTCOLOR_NORMAL);
    kqDraw.print_font(where, dx + 48, dy + 32, _("MP"), eFontColor::FONTCOLOR_GOLD);
    kqDraw.print_font(where, dx + 104, dy + 32, "/", eFontColor::FONTCOLOR_NORMAL);
    sprintf(strbuf, "%d", party[player_index_in_party].mp);
    j = strlen(strbuf) * 8;
    kqDraw.print_font(where, dx + 104 - j, dy + 32, strbuf, eFontColor::FONTCOLOR_NORMAL);
    sprintf(strbuf, "%d", party[player_index_in_party].mmp);
    j = strlen(strbuf) * 8;
    kqDraw.print_font(where, dx + 144 - j, dy + 32, strbuf, eFontColor::FONTCOLOR_NORMAL);
}

/*! \brief Add experience to stats
 *
 * Yep.
 *
 * \param   pl Player
 * \param   the_xp Amount of experience to give
 * \param   ls Learned new spell (always 1?)
 * \returns whether or not player raised levels
 */
int give_xp(int pl, int the_xp, int ls)
{
    party[pl].xp += the_xp;
    return KqFork::check_xp(pl, ls);
}

/*! \brief Add a new quest into the list
 *
 * \param   l - Array for the list we'll modify
 * \param   key - Title of the item
 * \param   text - Text to associate with the quest
 */
void KqFork::ilist_add(info_list* l, const char* key, const char* text)
{
    if (l->count >= l->capacity)
    {
        if (l->capacity == 0)
        {
            l->capacity = 10;
        }
        else
        {
            l->capacity *= 2;
        }
        l->root = (info_item*)realloc(l->root, l->capacity * sizeof(info_item));
    }
    l->root[l->count].key = strcpy((char*)malloc(strlen(key) + 1), key);
    l->root[l->count].text = strcpy((char*)malloc(strlen(text) + 1), text);
    ++l->count;
}

/*! \brief Remove all items
 *
 * Remove all items from the array
 * \sa ILIST
 * \author PH
 * \date 20050429
 */
static void ilist_clear(info_list* l)
{
    int i;

    for (i = 0; i < l->count; ++i)
    {
        free(l->root[i].key);
        free(l->root[i].text);
    }
    l->count = 0;
}

/*! \brief Levels up player
 *
 * Performs a level-up.
 *
 * \param   pr - Person leveling up
 */
void KqFork::level_up(int pr)
{
    int a, b = 0;
    float z;
    int bxp, xpi;
    KFighter tmpf;
    unsigned short* lup = party[pr].lup;

    player2fighter(pr, tmpf);
    xpi = lup[0];
    bxp = lup[1];
    party[pr].lvl++;
    a = party[pr].lvl + 1;
    z = ((a / 3) + (xpi * (a / 20 + 1) - 1)) * (((a - 2) / 2) * (a - 1));
    z += (bxp * (a / 20 + 1) * (a - 1));
    party[pr].next += (int)z;
    a = (kqrandom->random_range_exclusive(0, lup[2] / 2)) + lup[2] + (tmpf.fighterStats[A_VIT] / 5);
    party[pr].hp += a;
    party[pr].mhp += a;
    b = (kqrandom->random_range_exclusive(0, lup[3] / 2)) + lup[3];
    b += (tmpf.fighterStats[A_INT] + tmpf.fighterStats[A_SAG]) / 25;
    party[pr].mp += b;
    party[pr].mmp += b;
}

/*! \brief Main menu
 *
 * Main menu that calls all the other little menus :)
 */
void menu()
{
    int stop = 0, ptr = 0, z = -1;

    play_effect(SND_MENU, 128);
    timer_count = 0;
    while (!stop)
    {
        Game.do_check_animation();
        kqDraw.drawmap();
        draw_mainmenu(-1);
        draw_sprite(double_buffer, menuptr, 204 + xofs, ptr * 8 + 73 + yofs);
        kqDraw.blit2screen(xofs, yofs);
        PlayerInput.readcontrols();
        if (PlayerInput.up)
        {
            Game.unpress();
            ptr--;
            if (ptr < 0)
            {
                ptr = 5;
            }
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.down)
        {
            Game.unpress();
            ptr++;
            if (ptr > 5)
            {
                ptr = 0;
            }
            play_effect(SND_CLICK, 128);
        }
        /* Allow player to rearrange the party at any time by pressing LEFT */
        if (PlayerInput.left)
        {
            z = select_player();
            if (z > 0)
            {
                party_newlead();
            }
        }
        if (PlayerInput.balt)
        {
            Game.unpress();
            switch (ptr)
            {
            case 0:
                camp_item_menu();
                break;
            case 3:
                spec_items();
                break;
            case 5:
                KqFork::quest_info();
                break;
            default:
                z = select_player();
                if (z >= 0)
                {
                    switch (ptr)
                    {
                    case 1:
                        camp_spell_menu(z);
                        break;
                    case 2:
                        equip_menu(z);
                        break;
                    case 4:
                        KqFork::status_screen(z);
                        break;
                    }
                }
                break;
            }
        }
        if (PlayerInput.bctrl)
        {
            Game.unpress();
            stop = 1;
        }
        if (close_menu == 1)
        {
            close_menu = 0;
            stop = 1;
        }
    }
}

/* \brief Convert character-->fighter structure
 *
 * This function converts from the party structure to fighter structure.
 * Pass the character index and the function returns a fighter structure.
 * This is used for combat and for menu functions.
 * PH modified 20030308 I didn't like the way this returned a structure by
 * value.
 *
 * \param [in]  partyIndex - Index of player to convert
 * \param [out] outFighter - Converted fighter if return is true. Unchanged if
 * false.
 */
bool player2fighter(size_t partyIndex, KFighter& outFighter)
{
    if (partyIndex >= MAXCHRS)
    {
        return false;
    }
    KPlayer& playerInParty = party[partyIndex];

    KFighter fighterFromPlayer;
    fighterFromPlayer.imb_s = 0;
    fighterFromPlayer.imb_a = 0;
    fighterFromPlayer.imb[0] = 0;
    fighterFromPlayer.imb[1] = 0;
    fighterFromPlayer.fighterName = playerInParty.playerName.c_str();
    fighterFromPlayer.fighterExperience = playerInParty.xp;
    fighterFromPlayer.fighterLevel = playerInParty.lvl;
    fighterFromPlayer.fighterHealth = playerInParty.hp;
    fighterFromPlayer.fighterMaxHealth = playerInParty.mhp;
    fighterFromPlayer.fighterMagic = playerInParty.mp;
    fighterFromPlayer.fighterMaxMagic = playerInParty.mmp;
    for (int j = 0; j < 8; j++)
    {
        fighterFromPlayer.fighterSpellEffectStats[j] = playerInParty.sts[j];
    }
    for (int j = 8; j < NUM_SPELL_TYPES; j++)
    {
        fighterFromPlayer.fighterSpellEffectStats[j] = 0;
    }
    for (int j = 0; j < NUM_ATTRIBUTES; j++)
    {
        fighterFromPlayer.fighterStats[j] = ((playerInParty.lvl - 1) * playerInParty.lup[j + 4] + playerInParty.stats[j]) / 100;
    }
    for (int j = 0; j < R_TOTAL_RES; j++)
    {
        fighterFromPlayer.fighterResistance[j] = playerInParty.res[j];
    }

    /* set weapon elemental power and imbuements for easy use in combat */
    int weapon_index = playerInParty.eqp[EQP_WEAPON];
    fighterFromPlayer.welem = items[weapon_index].elem;
    if (items[weapon_index].use == USE_ATTACK)
    {
        fighterFromPlayer.imb_s = items[weapon_index].imb;
        fighterFromPlayer.imb_a = items[weapon_index].stats[A_ATT];
    }

    /* Set instants for equipment... these are imbuements that
     * take effect at the start of combat.  Technically, there
     * are only two imbue slots but there are five pieces of equipment
     * that can be imbued, so some item types get priority over
     * others... hence the need to run through each in this loop.
     */
    for (int a = 0; a < 5; a++)
    {
        static const int z[5] = { EQP_SPECIAL, EQP_ARMOR, EQP_HELMET, EQP_SHIELD, EQP_HAND };
        int current_equipment_slot = playerInParty.eqp[z[a]];
        if (items[current_equipment_slot].use == USE_IMBUED)
        {
            for (int b = 0; b < 2; b++)
            {
                if (fighterFromPlayer.imb[b] == 0)
                {
                    fighterFromPlayer.imb[b] = items[current_equipment_slot].imb;
                    b = 2;
                }
            }
        }
    }

    /*
     * Any weapon used by Ajathar gains the power of White if
     * it has no other power to begin with (the "welem" property
     * is 1-based: value of 0 means "no imbue".
     */
    if (partyIndex == AJATHAR && fighterFromPlayer.welem == 0)
    {
        fighterFromPlayer.welem = R_WHITE + 1;
    }
    for (int j = 0; j < NUM_EQUIPMENT; j++)
    {
        int a = playerInParty.eqp[j];
        if (j == 0)
        {
            if (a == 0)
            {
                fighterFromPlayer.bonus = 50;
            }
            else
            {
                fighterFromPlayer.bonus = items[a].bon;
            }
            if (items[a].icon == 1 || items[a].icon == 3 || items[a].icon == 21)
            {
                fighterFromPlayer.bstat = 1;
            }
            else
            {
                fighterFromPlayer.bstat = 0;
            }
            /* Set current weapon type. When the hero wields a weapon
             * in combat, it will look like this.
             * The color comes from s_item::kol
             */
            fighterFromPlayer.current_weapon_type = items[a].icon;
            if (fighterFromPlayer.current_weapon_type == W_CHENDIGAL)
            {
                fighterFromPlayer.current_weapon_type = W_SWORD;
            }
        }
        for (int b = 0; b < NUM_STATS; b++)
        {
            if (b == A_SPI && partyIndex == TEMMIN)
            {
                if (items[a].stats[A_SPI] > 0)
                {
                    fighterFromPlayer.fighterStats[A_SPI] += items[a].stats[A_SPI];
                }
            }
            else
            {
                fighterFromPlayer.fighterStats[b] += items[a].stats[b];
            }
        }
        for (int b = 0; b < R_TOTAL_RES; b++)
        {
            fighterFromPlayer.fighterResistance[b] += items[a].res[b];
        }
    }
    if (partyIndex == CORIN)
    {
        fighterFromPlayer.fighterResistance[R_EARTH] += fighterFromPlayer.fighterLevel / 4;
        fighterFromPlayer.fighterResistance[R_FIRE] += fighterFromPlayer.fighterLevel / 4;
        fighterFromPlayer.fighterResistance[R_AIR] += fighterFromPlayer.fighterLevel / 4;
        fighterFromPlayer.fighterResistance[R_WATER] += fighterFromPlayer.fighterLevel / 4;
    }
    if (playerInParty.eqp[5] == I_AGRAN)
    {
        int a = 0;
        for (int j = 0; j < R_TOTAL_RES; j++)
        {
            a += fighterFromPlayer.fighterResistance[j];
        }
        int b = ((a * 10) / 16 + 5) / 10;
        for (int j = 0; j < R_TOTAL_RES; j++)
        {
            fighterFromPlayer.fighterResistance[j] = b;
        }
    }
    for (int j = 0; j < 8; j++)
    {
        if (fighterFromPlayer.fighterResistance[j] < -10)
        {
            fighterFromPlayer.fighterResistance[j] = -10;
        }
        if (fighterFromPlayer.fighterResistance[j] > 20)
        {
            fighterFromPlayer.fighterResistance[j] = 20;
        }
    }
    for (int j = 8; j < R_TOTAL_RES; j++)
    {
        if (fighterFromPlayer.fighterResistance[j] < 0)
        {
            fighterFromPlayer.fighterResistance[j] = 0;
        }
        if (fighterFromPlayer.fighterResistance[j] > 10)
        {
            fighterFromPlayer.fighterResistance[j] = 10;
        }
    }
    if (playerInParty.eqp[5] == I_MANALOCKET)
    {
        fighterFromPlayer.mrp = playerInParty.mrp / 2;
    }
    else
    {
        fighterFromPlayer.mrp = playerInParty.mrp;
    }
    fighterFromPlayer.fighterStats[A_HIT] += fighterFromPlayer.fighterStats[A_STR] / 5;
    fighterFromPlayer.fighterStats[A_HIT] += fighterFromPlayer.fighterStats[A_AGI] / 5;
    fighterFromPlayer.fighterStats[A_DEF] += fighterFromPlayer.fighterStats[A_VIT] / 8;
    fighterFromPlayer.fighterStats[A_EVD] += fighterFromPlayer.fighterStats[A_AGI] / 5;
    fighterFromPlayer.fighterStats[A_MAG] += (fighterFromPlayer.fighterStats[A_INT] + fighterFromPlayer.fighterStats[A_SAG]) / 20;
    for (int j = 0; j < NUM_STATS; j++)
    {
        if (fighterFromPlayer.fighterStats[j] < 1)
        {
            fighterFromPlayer.fighterStats[j] = 1;
        }
    }
    fighterFromPlayer.fighterCanCriticalHit = 1;
    fighterFromPlayer.aux = 0;
    fighterFromPlayer.unl = 0;
    outFighter = fighterFromPlayer;
    return true;
}

/*! \brief Do the Quest Info menu
 *
 * Show the current list of quest information items
 * \sa ILIST
 * \author PH
 * \date 20050429
 */
void KqFork::quest_info()
{
    int ii = 0;
    int i, base;

    /* Call into the script */
    ilist_clear(&KqFork::quest_list);
    do_questinfo();
    if (KqFork::quest_list.count == 0)
    {
        /* There was nothing.. */
        play_effect(SND_BAD, 128);
        return;
    }

    while (1)
    {
        timer_count = 0;
        kqDraw.drawmap();
        base = ii - ii % 10;
        kqDraw.menubox(double_buffer, 88 + xofs, 92 + yofs, 18, 10, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        kqDraw.menubox(double_buffer, 88 + xofs, 188 + yofs, 18, 3, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        for (i = 0; i < 10; ++i)
        {
            if (i + base < KqFork::quest_list.count)
            {
                kqDraw.print_font(double_buffer, 104 + xofs, 100 + 8 * i + yofs, KqFork::quest_list.root[i + base].key, eFontColor::FONTCOLOR_NORMAL);
            }
        }
        draw_sprite(double_buffer, menuptr, 88 + xofs, 100 + 8 * (ii - base) + yofs);
        if (ii < KqFork::quest_list.count)
        {
            kqDraw.print_font(double_buffer, 96 + xofs, 196 + yofs, KqFork::quest_list.root[ii].text, eFontColor::FONTCOLOR_NORMAL);
        }
        kqDraw.blit2screen(xofs, yofs);
        PlayerInput.readcontrols();
        if (PlayerInput.up)
        {
            --ii;
            play_effect(SND_CLICK, 128);
            Game.unpress();
        }
        if (PlayerInput.down)
        {
            ++ii;
            play_effect(SND_CLICK, 128);
            Game.unpress();
        }
        if (PlayerInput.left)
        {
            ii -= 10;
            play_effect(SND_CLICK, 128);
            Game.unpress();
        }
        if (PlayerInput.right)
        {
            ii += 10;
            play_effect(SND_CLICK, 128);
            Game.unpress();
        }
        if (ii < 0)
        {
            ii = KqFork::quest_list.count - 1;
        }
        if (ii >= KqFork::quest_list.count)
        {
            ii = 0;
        }
        if (PlayerInput.balt || PlayerInput.bctrl)
        {
            Game.unpress();
            return;
        }
    }
}

/*! \brief Converts fighter-->character
 *
 * This converts from fighter to player structure.  Used when leaving combat.
 */
void revert_equipstats()
{
    size_t fighter_index, end_fighter_index;
    size_t pidx_index;
    size_t stats_index;

    if (numchrs > MAX_PARTY_SIZE)
    {
        end_fighter_index = MAX_PARTY_SIZE;
    }
    else
    {
        end_fighter_index = numchrs;
    }
    for (fighter_index = 0; fighter_index < end_fighter_index; fighter_index++)
    {
        pidx_index = pidx[fighter_index];
        party[pidx_index].hp = fighter[fighter_index].fighterHealth;
        if (party[pidx_index].hp > party[pidx_index].mhp)
        {
            party[pidx_index].hp = party[pidx_index].mhp;
        }
        party[pidx_index].mp = fighter[fighter_index].fighterMagic;
        if (party[pidx_index].mp > party[pidx_index].mmp)
        {
            party[pidx_index].mp = party[pidx_index].mmp;
        }
        for (stats_index = 0; stats_index < 12; stats_index++)
        {
            party[pidx_index].sts[stats_index] = 0;
        }
        party[pidx_index].sts[S_POISON] = fighter[fighter_index].fighterSpellEffectStats[S_POISON];
        party[pidx_index].sts[S_BLIND] = fighter[fighter_index].fighterSpellEffectStats[S_BLIND];
        party[pidx_index].sts[S_MUTE] = fighter[fighter_index].fighterSpellEffectStats[S_MUTE];
        party[pidx_index].sts[S_DEAD] = fighter[fighter_index].fighterSpellEffectStats[S_DEAD];
        for (stats_index = 0; stats_index < 12; stats_index++)
        {
            if (stats_index != S_POISON && stats_index != S_BLIND && stats_index != S_MUTE && stats_index != S_DEAD)
            {
                fighter[fighter_index].fighterSpellEffectStats[stats_index] = 0;
            }
        }
    }
}

/*! \brief Show special items
 *
 * List any special items that the party has.
 *
 * WK: This function would be more appropriate in a script, such as global.lua.
 * This function is preventing me from completely removing progress.h
 */
void spec_items()
{
    int a, num_items = 0, stop = 0, ptr = 0;
    short list_item_which[MAX_PLAYER_SPECIAL_ITEMS];
    short list_item_quantity[MAX_PLAYER_SPECIAL_ITEMS];

    /* Set number of items here */
    for (a = 0; a < MAX_SPECIAL_ITEMS; a++)
    {
        if (player_special_items[a])
        {
            list_item_which[num_items] = a;
            list_item_quantity[num_items] = player_special_items[a];
            num_items++;
        }
    }

    if (num_items == 0)
    {
        play_effect(SND_BAD, 128);
        return;
    }
    play_effect(SND_MENU, 128);
    while (!stop)
    {
        Game.do_check_animation();
        kqDraw.drawmap();
        kqDraw.menubox(double_buffer, 72 + xofs, 12 + yofs, 20, 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        kqDraw.print_font(double_buffer, 108 + xofs, 20 + yofs, _("Special Items"), eFontColor::FONTCOLOR_GOLD);
        kqDraw.menubox(double_buffer, 72 + xofs, 36 + yofs, 20, 19, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        for (a = 0; a < num_items; a++)
        {
            kqDraw.draw_icon(double_buffer, special_items[list_item_which[a]].icon, 88 + xofs, a * 8 + 44 + yofs);
            kqDraw.print_font(double_buffer, 96 + xofs, a * 8 + 44 + yofs, special_items[list_item_which[a]].specialItemName, eFontColor::FONTCOLOR_NORMAL);
            if (list_item_quantity[a] > 1)
            {
                sprintf(strbuf, "^%d", list_item_quantity[a]);
                kqDraw.print_font(double_buffer, 224 + xofs, a * 8 + 44 + yofs, strbuf, eFontColor::FONTCOLOR_NORMAL);
            }
        }
        kqDraw.menubox(double_buffer, 72 + xofs, 204 + yofs, 20, 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        a = strlen(special_items[list_item_which[ptr]].description) * 4;
        kqDraw.print_font(double_buffer, 160 - a + xofs, 212 + yofs, special_items[list_item_which[ptr]].description, eFontColor::FONTCOLOR_NORMAL);
        draw_sprite(double_buffer, menuptr, 72 + xofs, ptr * 8 + 44 + yofs);
        kqDraw.blit2screen(xofs, yofs);
        PlayerInput.readcontrols();

        if (PlayerInput.down)
        {
            Game.unpress();
            ptr = (ptr + 1) % num_items;
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.up)
        {
            Game.unpress();
            ptr = (ptr - 1 + num_items) % num_items;
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.bctrl)
        {
            Game.unpress();
            stop = 1;
        }
    }
}

/*! \brief Draw a player's status screen
 *
 * Draw the verbose stats of a single player.
 * \param   fighter_index - Character to draw (index in pidx array)
 */
void KqFork::status_screen(size_t fighter_index)
{
    int stop = 0;
    int bc = 0;
    uint32_t rect_fill_amount = 0, curr_fill, res_index, stats_y, equipment_index;
    size_t pidx_index, stats_index;

    play_effect(SND_MENU, 128);
    pidx_index = pidx[fighter_index];
    update_equipstats();
    while (!stop)
    {
        Game.do_check_animation();
        // Redraw the map, clearing any menus under this new window
        kqDraw.drawmap();

        // Box around top-left square
        kqDraw.menubox(double_buffer, xofs, 16 + yofs, 18, 5, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        draw_playerstat(double_buffer, pidx_index, 8 + xofs, 24 + yofs);

        // Box around bottom-left square
        kqDraw.menubox(double_buffer, xofs, 72 + yofs, 18, 17, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        kqDraw.print_font(double_buffer, 8 + xofs, 80 + yofs, _("Exp:"), eFontColor::FONTCOLOR_GOLD);
        sprintf(strbuf, "%d", party[pidx_index].xp);
        kqDraw.print_font(double_buffer, 152 - (strlen(strbuf) * 8) + xofs, 80 + yofs, strbuf, eFontColor::FONTCOLOR_NORMAL);
        kqDraw.print_font(double_buffer, 8 + xofs, 88 + yofs, _("Next:"), eFontColor::FONTCOLOR_GOLD);
        // TT: Does this mean we can only level up to 50?
        if (party[pidx_index].lvl < 50)
        {
            sprintf(strbuf, "%d", party[pidx_index].next - party[pidx_index].xp);
        }
        else
        {
            sprintf(strbuf, "%d", 0);
        }
        kqDraw.print_font(double_buffer, 152 - (strlen(strbuf) * 8) + xofs, 88 + yofs, strbuf, eFontColor::FONTCOLOR_NORMAL);
        kqDraw.print_font(double_buffer, 8 + xofs, 104 + yofs, _("Strength"), eFontColor::FONTCOLOR_GOLD);
        kqDraw.print_font(double_buffer, 8 + xofs, 112 + yofs, _("Agility"), eFontColor::FONTCOLOR_GOLD);
        kqDraw.print_font(double_buffer, 8 + xofs, 120 + yofs, _("Vitality"), eFontColor::FONTCOLOR_GOLD);
        kqDraw.print_font(double_buffer, 8 + xofs, 128 + yofs, _("Intellect"), eFontColor::FONTCOLOR_GOLD);
        kqDraw.print_font(double_buffer, 8 + xofs, 136 + yofs, _("Sagacity"), eFontColor::FONTCOLOR_GOLD);
        kqDraw.print_font(double_buffer, 8 + xofs, 144 + yofs, _("Speed"), eFontColor::FONTCOLOR_GOLD);
        kqDraw.print_font(double_buffer, 8 + xofs, 152 + yofs, _("Aura"), eFontColor::FONTCOLOR_GOLD);
        kqDraw.print_font(double_buffer, 8 + xofs, 160 + yofs, _("Spirit"), eFontColor::FONTCOLOR_GOLD);
        // Blank space on display of 16 pixels
        kqDraw.print_font(double_buffer, 8 + xofs, 176 + yofs, _("Attack"), eFontColor::FONTCOLOR_GOLD);
        kqDraw.print_font(double_buffer, 8 + xofs, 184 + yofs, _("Hit"), eFontColor::FONTCOLOR_GOLD);
        kqDraw.print_font(double_buffer, 8 + xofs, 192 + yofs, _("Defense"), eFontColor::FONTCOLOR_GOLD);
        kqDraw.print_font(double_buffer, 8 + xofs, 200 + yofs, _("Evade"), eFontColor::FONTCOLOR_GOLD);
        kqDraw.print_font(double_buffer, 8 + xofs, 208 + yofs, _("Mag.Def"), eFontColor::FONTCOLOR_GOLD);
        for (stats_index = 0; stats_index < NUM_STATS; stats_index++)
        {
            // Coordinates of stats on display
            stats_y = stats_index * 8 + 104;
            // Add an extra 8-pixel space to separate these from the others
            if (stats_index > A_SPI)
            {
                stats_y += 8;
            }
            kqDraw.print_font(double_buffer, 96 + xofs, stats_y + yofs, "$", eFontColor::FONTCOLOR_GOLD);
            sprintf(strbuf, "%d", fighter[fighter_index].fighterStats[stats_index]);
            kqDraw.print_font(double_buffer, 152 - (strlen(strbuf) * 8) + xofs, stats_y + yofs, strbuf, eFontColor::FONTCOLOR_NORMAL);
        }

        kqDraw.menubox(double_buffer, 160 + xofs, 16 + yofs, 18, 16, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        kqDraw.print_font(double_buffer, 168 + xofs, 24 + yofs, _("Earth"), eFontColor::FONTCOLOR_NORMAL);
        kqDraw.print_font(double_buffer, 168 + xofs, 32 + yofs, _("Black"), eFontColor::FONTCOLOR_NORMAL);
        kqDraw.print_font(double_buffer, 168 + xofs, 40 + yofs, _("Fire"), eFontColor::FONTCOLOR_NORMAL);
        kqDraw.print_font(double_buffer, 168 + xofs, 48 + yofs, _("Thunder"), eFontColor::FONTCOLOR_NORMAL);
        kqDraw.print_font(double_buffer, 168 + xofs, 56 + yofs, _("Air"), eFontColor::FONTCOLOR_NORMAL);
        kqDraw.print_font(double_buffer, 168 + xofs, 64 + yofs, _("White"), eFontColor::FONTCOLOR_NORMAL);
        kqDraw.print_font(double_buffer, 168 + xofs, 72 + yofs, _("Water"), eFontColor::FONTCOLOR_NORMAL);
        kqDraw.print_font(double_buffer, 168 + xofs, 80 + yofs, _("Ice"), eFontColor::FONTCOLOR_NORMAL);
        kqDraw.print_font(double_buffer, 168 + xofs, 88 + yofs, _("Poison"), eFontColor::FONTCOLOR_NORMAL);
        kqDraw.print_font(double_buffer, 168 + xofs, 96 + yofs, _("Blind"), eFontColor::FONTCOLOR_NORMAL);
        kqDraw.print_font(double_buffer, 168 + xofs, 104 + yofs, _("Charm"), eFontColor::FONTCOLOR_NORMAL);
        kqDraw.print_font(double_buffer, 168 + xofs, 112 + yofs, _("Paralyze"), eFontColor::FONTCOLOR_NORMAL);
        kqDraw.print_font(double_buffer, 168 + xofs, 120 + yofs, _("Petrify"), eFontColor::FONTCOLOR_NORMAL);
        kqDraw.print_font(double_buffer, 168 + xofs, 128 + yofs, _("Silence"), eFontColor::FONTCOLOR_NORMAL);
        kqDraw.print_font(double_buffer, 168 + xofs, 136 + yofs, _("Sleep"), eFontColor::FONTCOLOR_NORMAL);
        kqDraw.print_font(double_buffer, 168 + xofs, 144 + yofs, _("Time"), eFontColor::FONTCOLOR_NORMAL);

        for (res_index = 0; res_index < R_TOTAL_RES; res_index++)
        {
            rectfill(double_buffer, 240 + xofs, res_index * 8 + 25 + yofs, 310 + xofs, res_index * 8 + 31 + yofs, 3);
            if (fighter[fighter_index].fighterResistance[res_index] < 0)
            {
                bc = 18; // bright red, meaning WEAK defense
                rect_fill_amount = abs(fighter[fighter_index].fighterResistance[res_index]);
            }
            else if (fighter[fighter_index].fighterResistance[res_index] >= 0 && fighter[fighter_index].fighterResistance[res_index] <= 10)
            {
                bc = 34; // bright green, meaning so-so defense
                rect_fill_amount = fighter[fighter_index].fighterResistance[res_index];
            }
            else if (fighter[fighter_index].fighterResistance[res_index] > 10)
            {
                bc = 50; // bright blue, meaning STRONG defense
                rect_fill_amount = fighter[fighter_index].fighterResistance[res_index] - 10;
            }

            if (rect_fill_amount > 0)
            {
                for (curr_fill = 0; curr_fill < rect_fill_amount; curr_fill++)
                {
                    rectfill(double_buffer, curr_fill * 7 + 241 + xofs, res_index * 8 + 26 + yofs, curr_fill * 7 + 246 + xofs, res_index * 8 + 30 + yofs, bc + curr_fill);
                }
            }
        }
        kqDraw.menubox(double_buffer, 160 + xofs, 160 + yofs, 18, 6, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        for (equipment_index = 0; equipment_index < NUM_EQUIPMENT; equipment_index++)
        {
            kqDraw.draw_icon(double_buffer, items[party[pidx_index].eqp[equipment_index]].icon, 168 + xofs, equipment_index * 8 + 168 + yofs);
            kqDraw.print_font(double_buffer, 176 + xofs, equipment_index * 8 + 168 + yofs, items[party[pidx_index].eqp[equipment_index]].itemName, eFontColor::FONTCOLOR_NORMAL);
        }
        kqDraw.blit2screen(xofs, yofs);
        PlayerInput.readcontrols();

        if (PlayerInput.left && fighter_index > 0)
        {
            Game.unpress();
            fighter_index--;
            pidx_index = pidx[fighter_index];
            play_effect(SND_MENU, 128);
        }
        if (PlayerInput.right && fighter_index < numchrs - 1)
        {
            Game.unpress();
            fighter_index++;
            pidx_index = pidx[fighter_index];
            play_effect(SND_MENU, 128);
        }
        if (PlayerInput.bctrl)
        {
            Game.unpress();
            play_effect(SND_MENU, 128);
            stop = 1;
        }
    }
}

/*! \brief Copy party-->fighter structure
 *
 * Just used to convert all characters in party from party structure
 * to fighter structure.
 */
void update_equipstats()
{
    size_t fighter_index;

    for (fighter_index = 0; fighter_index < numchrs; fighter_index++)
    {
        player2fighter(pidx[fighter_index], fighter[fighter_index]);
    }
}
