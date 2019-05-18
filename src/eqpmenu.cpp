/*! \file
 * \brief Equipment menu stuff
 * \author JB
 * \date ????????
 *
 * This file contains code to handle the equipment menu
 * including dropping and optimizing the items carried.
 */

#include <stdio.h>
#include <string.h>

#include "draw.h"
#include "eqpmenu.h"
#include "gfx.h"
#include "input.h"
#include "itemmenu.h"
#include "kq.h"
#include "menu.h"
#include "res.h"
#include "setup.h"

/* Globals  */
int tstats[13], tres[R_TOTAL_RES];
uint16_t t_inv[MAX_INV], sm;
size_t tot;
char eqp_act;

/* Internal functions */
static void draw_equipmenu(int, int);
static void draw_equippable(uint32_t, uint32_t, uint32_t);
static void calc_possible_equip(int, int);
static void optimize_equip(int);
static void choose_equipment(int, int);
static void calc_equippreview(uint32_t, uint32_t, int);
static void draw_equippreview(int, int, int);
static int equip(uint32_t, uint32_t, uint32_t);
static int deequip(uint32_t, uint32_t);

/*! \brief Show the effect on stats if this piece were selected
 *
 * This is used to calculate the difference in stats due to
 * (de)equipping a piece of equipment.
 *
 * \param   aa Character to process
 * \param   p2 Slot to consider changing
 * \param   ii New piece of equipment to compare/use
 */
static void calc_equippreview(uint32_t aa, uint32_t p2, int ii)
{
    int c, z;

    c = party[pidx[aa]].eqp[p2];
    party[pidx[aa]].eqp[p2] = ii;
    update_equipstats();
    for (z = 0; z < 13; z++)
    {
        tstats[z] = fighter[aa].fighterStats[z];
    }
    for (z = 0; z < R_TOTAL_RES; z++)
    {
        tres[z] = fighter[aa].fighterResistance[z];
    }
    party[pidx[aa]].eqp[p2] = c;
    update_equipstats();
}

/*! \brief List equipment that can go in a slot
 *
 * Create a list of equipment that can be equipped in a particular
 * slot for a particular hero.
 *
 * \param   c Character to equip
 * \param   slot Which body part to equip
 */
static void calc_possible_equip(int c, int slot)
{
    uint32_t k;

    tot = 0;
    for (k = 0; k < MAX_INV; k++)
    {
        // Check if we have any items at all
        if (g_inv[k].item > 0 && g_inv[k].quantity > 0)
        {
            if (items[g_inv[k].item].type == slot && items[g_inv[k].item].eq[pidx[c]] != 0)
            {
                t_inv[tot] = k;
                tot++;
            }
        }
    }
}

/*! \brief Handle selecting an equipment item
 *
 * After choosing an equipment slot, select an item to equip
 *
 * \param   c Character to equip
 * \param   slot Which part of the body to process
 */
static void choose_equipment(int c, int slot)
{
    int stop = 0, yptr = 0, pptr = 0, sm = 0, ym = 15;

    while (!stop)
    {
        Game.do_check_animation();
        kqDraw.drawmap();
        draw_equipmenu(c, 0);
        draw_equippable(c, slot, pptr);
        if (tot == 0)
        {
            draw_equippreview(c, -1, 0);
            play_effect(SND_BAD, 128);
            return;
        }
        draw_equippreview(c, slot, g_inv[t_inv[pptr + yptr]].item);
        draw_sprite(double_buffer, menuptr, 12 + xofs, yptr * 8 + 100 + yofs);
        kqDraw.blit2screen(xofs, yofs);
        if (tot < NUM_ITEMS_PER_PAGE)
        {
            sm = 0;
            ym = tot - 1;
        }
        else
        {
            sm = tot - NUM_ITEMS_PER_PAGE;
        }

        PlayerInput.readcontrols();

        if (PlayerInput.down)
        {
            Game.unpress();
            if (yptr == 15)
            {
                pptr++;
                if (pptr > sm)
                {
                    pptr = sm;
                }
            }
            else
            {
                if (yptr < ym)
                {
                    yptr++;
                }
            }
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.up)
        {
            Game.unpress();
            if (yptr == 0)
            {
                pptr--;
                if (pptr < 0)
                {
                    pptr = 0;
                }
            }
            else
            {
                yptr--;
            }
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.balt)
        {
            Game.unpress();
            if (equip(pidx[c], t_inv[pptr + yptr], 0) == 1)
            {
                play_effect(SND_EQUIP, 128);
                stop = 1;
            }
            else
            {
                play_effect(SND_BAD, 128);
            }
        }
        if (PlayerInput.bctrl)
        {
            Game.unpress();
            stop = 1;
        }
    }
    return;
}

/*! \brief Check if item can be de-equipped, then do it.
 *
 * Hmm... this is hard to describe :)  The functions makes sure you have
 * room to de-equip before it actual does anything.
 *
 * \param   c Character to process
 * \param   ptr Slot to de-equip
 * \returns 0 if unsuccessful, 1 if successful
 */
static int deequip(uint32_t c, uint32_t ptr)
{
    int a, b = 0;

    if (ptr >= NUM_EQUIPMENT)
    {
        return 0;
    }

    a = party[pidx[c]].eqp[ptr];
    if (a > 0)
    {
        b = check_inventory(a, 1);
    }
    else
    {
        return 0;
    }

    if (b == 0)
    {
        return 0;
    }
    party[pidx[c]].eqp[ptr] = 0;
    return 1;
}

/*! \brief Draw the equipment menu
 *
 * This is simply a function to display the equip menu screen.
 * It's kept separate from the equip_menu routine for the sake
 * of code cleanliness... better late than never :P
 *
 * \param   c Index of character to equip
 * \param   sel If sel==1, show the full range of options (Equip, Optimize,
 *              Remove, Empty)
 *              Otherwise just show Equip if eqp_act is 0 or Remove if it is 2.
 *              (This is when you're selecting the item to Equip/Remove)
 */
static void draw_equipmenu(int c, int sel)
{
    int l, j, k;

    l = pidx[c];
    kqDraw.menubox(double_buffer, 12 + xofs, 4 + yofs, 35, 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
    if (sel == 1)
    {
        kqDraw.menubox(double_buffer, eqp_act * 72 + 12 + xofs, 4 + yofs, 8, 1, eMenuBoxColor::DARKBLUE);
        kqDraw.print_font(double_buffer, 32 + xofs, 12 + yofs, _("Equip"), eFontColor::FONTCOLOR_GOLD);
        kqDraw.print_font(double_buffer, 92 + xofs, 12 + yofs, _("Optimize"), eFontColor::FONTCOLOR_GOLD);
        kqDraw.print_font(double_buffer, 172 + xofs, 12 + yofs, _("Remove"), eFontColor::FONTCOLOR_GOLD);
        kqDraw.print_font(double_buffer, 248 + xofs, 12 + yofs, _("Empty"), eFontColor::FONTCOLOR_GOLD);
    }
    else
    {
        if (eqp_act == 0)
        {
            kqDraw.print_font(double_buffer, 140 + xofs, 12 + yofs, _("Equip"), eFontColor::FONTCOLOR_GOLD);
        }
        if (eqp_act == 2)
        {
            kqDraw.print_font(double_buffer, 136 + xofs, 12 + yofs, _("Remove"), eFontColor::FONTCOLOR_GOLD);
        }
    }
    kqDraw.menubox(double_buffer, 12 + xofs, 28 + yofs, 25, 6, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
    kqDraw.menubox(double_buffer, 228 + xofs, 28 + yofs, 8, 6, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
    draw_sprite(double_buffer, players[l].portrait, 248 + xofs, 36 + yofs);
    kqDraw.print_font(double_buffer, 268 - (party[l].playerName.length() * 4) + xofs, 76 + yofs, party[l].playerName.c_str(), eFontColor::FONTCOLOR_NORMAL);
    kqDraw.print_font(double_buffer, 28 + xofs, 36 + yofs, _("Hand1:"), eFontColor::FONTCOLOR_GOLD);
    kqDraw.print_font(double_buffer, 28 + xofs, 44 + yofs, _("Hand2:"), eFontColor::FONTCOLOR_GOLD);
    kqDraw.print_font(double_buffer, 28 + xofs, 52 + yofs, _("Head:"), eFontColor::FONTCOLOR_GOLD);
    kqDraw.print_font(double_buffer, 28 + xofs, 60 + yofs, _("Body:"), eFontColor::FONTCOLOR_GOLD);
    kqDraw.print_font(double_buffer, 28 + xofs, 68 + yofs, _("Arms:"), eFontColor::FONTCOLOR_GOLD);
    kqDraw.print_font(double_buffer, 28 + xofs, 76 + yofs, _("Other:"), eFontColor::FONTCOLOR_GOLD);
    for (k = 0; k < NUM_EQUIPMENT; k++)
    {
        j = party[l].eqp[k];
        kqDraw.draw_icon(double_buffer, items[j].icon, 84 + xofs, k * 8 + 36 + yofs);
        kqDraw.print_font(double_buffer, 92 + xofs, k * 8 + 36 + yofs, items[j].itemName, eFontColor::FONTCOLOR_NORMAL);
    }
}

/*! \brief Draw list of items that can be used to equip this slot
 *
 * This displays the list of items that the character posesses.
 * However, items that the character can't equip in the slot
 * specified, are greyed out.
 *
 * \param   c Character to equip
 * \param   slot Which 'part of the body' to equip
 * \param   pptr Which page of the inventory to draw
 */
static void draw_equippable(uint32_t c, uint32_t slot, uint32_t pptr)
{
    int z, j, k;

    if (slot < NUM_EQUIPMENT)
    {
        calc_possible_equip(c, slot);
    }
    else
    {
        tot = 0;
    }
    if (tot < NUM_ITEMS_PER_PAGE)
    {
        sm = (uint16_t)tot;
    }
    else
    {
        sm = NUM_ITEMS_PER_PAGE;
    }
    kqDraw.menubox(double_buffer, 12 + xofs, 92 + yofs, 20, NUM_ITEMS_PER_PAGE, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
    for (k = 0; k < sm; k++)
    {
        // j == item index #
        j = g_inv[t_inv[pptr + k]].item;
        // z == number of items
        z = g_inv[t_inv[pptr + k]].quantity;
        kqDraw.draw_icon(double_buffer, items[j].icon, 28 + xofs, k * 8 + 100 + yofs);
        kqDraw.print_font(double_buffer, 36 + xofs, k * 8 + 100 + yofs, items[j].itemName, eFontColor::FONTCOLOR_NORMAL);
        if (z > 1)
        {
            sprintf(strbuf, "^%d", z);
            kqDraw.print_font(double_buffer, 164 + xofs, k * 8 + 100 + yofs, strbuf, eFontColor::FONTCOLOR_NORMAL);
        }
    }
    if (pptr > 0)
    {
        draw_sprite(double_buffer, upptr, 180 + xofs, 98 + yofs);
    }
    if (tot > NUM_ITEMS_PER_PAGE)
    {
        if (pptr < tot - NUM_ITEMS_PER_PAGE)
        {
            draw_sprite(double_buffer, dnptr, 180 + xofs, 206 + yofs);
        }
    }
}

/*! \brief Display changed stats
 *
 * This displays the results of the above function so that
 * players can tell how a piece of equipment will affect
 * their stats.
 *
 * \param   ch Character to process
 * \param   ptr Slot to change, or <0 to switch to new stats
 * \param   pp New item to use
 */
static void draw_equippreview(int ch, int ptr, int pp)
{
    int z, c1, c2;

    if (ptr >= 0)
    {
        calc_equippreview(ch, ptr, pp);
    }
    else
    {
        update_equipstats();
    }
    kqDraw.menubox(double_buffer, 188 + xofs, 92 + yofs, 13, 13, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
    kqDraw.print_font(double_buffer, 196 + xofs, 100 + yofs, _("Str:"), eFontColor::FONTCOLOR_NORMAL);
    kqDraw.print_font(double_buffer, 196 + xofs, 108 + yofs, _("Agi:"), eFontColor::FONTCOLOR_NORMAL);
    kqDraw.print_font(double_buffer, 196 + xofs, 116 + yofs, _("Vit:"), eFontColor::FONTCOLOR_NORMAL);
    kqDraw.print_font(double_buffer, 196 + xofs, 124 + yofs, _("Int:"), eFontColor::FONTCOLOR_NORMAL);
    kqDraw.print_font(double_buffer, 196 + xofs, 132 + yofs, _("Sag:"), eFontColor::FONTCOLOR_NORMAL);
    kqDraw.print_font(double_buffer, 196 + xofs, 140 + yofs, _("Spd:"), eFontColor::FONTCOLOR_NORMAL);
    kqDraw.print_font(double_buffer, 196 + xofs, 148 + yofs, _("Aur:"), eFontColor::FONTCOLOR_NORMAL);
    kqDraw.print_font(double_buffer, 196 + xofs, 156 + yofs, _("Spi:"), eFontColor::FONTCOLOR_NORMAL);
    kqDraw.print_font(double_buffer, 196 + xofs, 164 + yofs, _("Att:"), eFontColor::FONTCOLOR_NORMAL);
    kqDraw.print_font(double_buffer, 196 + xofs, 172 + yofs, _("Hit:"), eFontColor::FONTCOLOR_NORMAL);
    kqDraw.print_font(double_buffer, 196 + xofs, 180 + yofs, _("Def:"), eFontColor::FONTCOLOR_NORMAL);
    kqDraw.print_font(double_buffer, 196 + xofs, 188 + yofs, _("Evd:"), eFontColor::FONTCOLOR_NORMAL);
    kqDraw.print_font(double_buffer, 196 + xofs, 196 + yofs, _("Mdf:"), eFontColor::FONTCOLOR_NORMAL);
    for (z = 0; z < 13; z++)
    {
        c1 = fighter[ch].fighterStats[z];
        c2 = tstats[z];
        sprintf(strbuf, "%d", c1);
        kqDraw.print_font(double_buffer, 252 - (strlen(strbuf) * 8) + xofs, z * 8 + 100 + yofs, strbuf, eFontColor::FONTCOLOR_NORMAL);
        kqDraw.print_font(double_buffer, 260 + xofs, z * 8 + 100 + yofs, ">", eFontColor::FONTCOLOR_NORMAL);
        if (ptr >= 0)
        {
            sprintf(strbuf, "%d", c2);
            if (c1 < c2)
            {
                kqDraw.print_font(double_buffer, 300 - (strlen(strbuf) * 8) + xofs, z * 8 + 100 + yofs, strbuf, eFontColor::FONTCOLOR_GREEN);
            }
            if (c2 < c1)
            {
                kqDraw.print_font(double_buffer, 300 - (strlen(strbuf) * 8) + xofs, z * 8 + 100 + yofs, strbuf, eFontColor::FONTCOLOR_RED);
            }
            if (c1 == c2)
            {
                kqDraw.print_font(double_buffer, 300 - (strlen(strbuf) * 8) + xofs, z * 8 + 100 + yofs, strbuf, eFontColor::FONTCOLOR_NORMAL);
            }
        }
    }
    kqDraw.menubox(double_buffer, 188 + xofs, 212 + yofs, 13, 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
    if (ptr >= 0)
    {
        c1 = 0;
        c2 = 0;
        for (z = 0; z < R_TOTAL_RES; z++)
        {
            c1 += fighter[ch].fighterResistance[z];
            c2 += tres[z];
        }
        if (c1 < c2)
        {
            kqDraw.print_font(double_buffer, 212 + xofs, 220 + yofs, _("Resist up"), eFontColor::FONTCOLOR_NORMAL);
        }
        if (c1 > c2)
        {
            kqDraw.print_font(double_buffer, 204 + xofs, 220 + yofs, _("Resist down"), eFontColor::FONTCOLOR_NORMAL);
        }
    }
}

/*! \brief Change a character's equipment
 *
 * Do the actual equip.  Of course, it will de-equip anything that
 * is currently in the specified slot.  The 'forced' var is used to
 * give the character an item and equip it all in one shot.  This is
 * typically only used for scripted events, but could be added to the
 * shop menu processing, so that things are equipped as soon as you
 * buy them.
 *
 * \param   c Character to process
 * \param   selected_item Item to add
 * \param   forced Non-zero if character doesn't already have the item (see
 * above)
 * \returns 1 if equip was successful, 0 otherwise
 */
static int equip(uint32_t c, uint32_t selected_item, uint32_t forced)
{
    uint32_t a;
    int d, b, z, n = 0, i;

    if (selected_item >= MAX_INV)
    {
        return 0;
    }

    if (forced == 0)
    {
        d = g_inv[selected_item].item;
    }
    else
    {
        d = selected_item;
    }
    a = items[d].type;
    if (a < NUM_EQUIPMENT)
    {
        b = party[c].eqp[a];
    }
    else
    {
        return 0;
    }

    if (items[d].eq[c] == 0)
    {
        return 0;
    }
    if (a == EQP_SHIELD)
    {
        if (party[c].eqp[EQP_WEAPON] > 0 && items[party[c].eqp[EQP_WEAPON]].hnds == 1)
        {
            return 0;
        }
    }
    else if (a == EQP_WEAPON)
    {
        if (party[c].eqp[EQP_SHIELD] > 0 && items[d].hnds == 1)
        {
            return 0;
        }
    }
    if (b > 0)
    {
        for (i = 0; i < MAX_INV; i++)
        {
            // Check if we have any items at all
            if (g_inv[selected_item].item > 0 && g_inv[selected_item].quantity > 0)
            {
                n++;
            }
        }
        // this first argument checks to see if there's one of given item
        if (g_inv[selected_item].quantity == 1 && n == MAX_INV && forced == 0)
        {
            party[c].eqp[a] = d;
            g_inv[selected_item].item = b;
            g_inv[selected_item].quantity = 1;
            return 1;
        }
        else
        {
            z = check_inventory(b, 1);
            if (z != 0)
            {
                party[c].eqp[a] = 0;
            }
            else
            {
                return 0;
            }
        }
    }
    party[c].eqp[a] = d;
    if (forced == 0)
    {
        remove_item(selected_item, 1);
    }
    return 1;
}

/*! \brief Handle equip menu
 *
 * Draw the equip menu stuff and let the user select an equip slot.
 *
 * \param   c Character to process
 */
void equip_menu(uint32_t c)
{
    int stop = 0, yptr = 0, sl = 1;
    int a, b, d;

    eqp_act = 0;
    play_effect(SND_MENU, 128);
    while (!stop)
    {
        Game.do_check_animation();
        kqDraw.drawmap();
        draw_equipmenu(c, sl);
        if (sl == 0)
        {
            draw_equippable(c, yptr, 0);
            if (eqp_act == 2)
            {
                draw_equippreview(c, yptr, 0);
            }
            else
            {
                draw_equippreview(c, -1, 0);
            }
        }
        else
        {
            draw_equippable(c, (uint32_t) - 1, 0);
            draw_equippreview(c, -1, 0);
        }
        if (sl == 0)
        {
            draw_sprite(double_buffer, menuptr, 12 + xofs, yptr * 8 + 36 + yofs);
        }
        kqDraw.blit2screen(xofs, yofs);

        PlayerInput.readcontrols();

        if (sl == 1)
        {
            if (PlayerInput.left)
            {
                Game.unpress();
                eqp_act--;
                if (eqp_act < 0)
                {
                    eqp_act = 3;
                }
                play_effect(SND_CLICK, 128);
            }
            if (PlayerInput.right)
            {
                Game.unpress();
                eqp_act++;
                if (eqp_act > 3)
                {
                    eqp_act = 0;
                }
                play_effect(SND_CLICK, 128);
            }
        }
        else
        {
            if (PlayerInput.down)
            {
                Game.unpress();
                yptr++;
                if (yptr > 5)
                {
                    yptr = 0;
                }
                play_effect(SND_CLICK, 128);
            }
            if (PlayerInput.up)
            {
                Game.unpress();
                yptr--;
                if (yptr < 0)
                {
                    yptr = 5;
                }
                play_effect(SND_CLICK, 128);
            }
        }
        if (PlayerInput.balt)
        {
            Game.unpress();
            if (sl == 1)
            {
                // If the selection is over 'Equip' or 'Remove'
                if (eqp_act == 0 || eqp_act == 2)
                {
                    sl = 0;
                }
                else if (eqp_act == 1)
                {
                    optimize_equip(c);
                }
                else if (eqp_act == 3)
                {
                    b = 0;
                    d = 0;
                    for (a = 0; a < NUM_EQUIPMENT; a++)
                    {
                        if (party[pidx[c]].eqp[a] > 0)
                        {
                            d++;
                            b += deequip(c, a);
                        }
                    }
                    if (b == d)
                    {
                        play_effect(SND_UNEQUIP, 128);
                    }
                    else
                    {
                        play_effect(SND_BAD, 128);
                    }
                }
            }
            else
            {
                if (eqp_act == 0)
                {
                    choose_equipment(c, yptr);
                }
                else
                {
                    if (eqp_act == 2)
                    {
                        if (deequip(c, yptr) == 1)
                        {
                            play_effect(SND_UNEQUIP, 128);
                        }
                        else
                        {
                            play_effect(SND_BAD, 128);
                        }
                    }
                }
            }
        }
        if (PlayerInput.bctrl)
        {
            Game.unpress();
            if (sl == 0)
            {
                sl = 1;
            }
            else
            {
                stop = 1;
            }
        }
    }
}

/*! \brief Calculate optimum equipment
 *
 * This calculates what equipment is optimum for a particular hero.
 * The weapon that does the most damage is chosen and the armor with
 * the best combination of defense+magic_defense is chosen.  As for a
 * relic, the one that offers the greatest overall bonus to stats is
 * selected.
 *
 * \param   c Which character to operate on
 */
static void optimize_equip(int c)
{
    uint32_t a;
    int b, z, maxx, maxi, v = 0;

    for (a = 0; a < NUM_EQUIPMENT; a++)
    {
        if (party[pidx[c]].eqp[a] > 0)
        {
            if (deequip(c, a) == 0)
            {
                return;
            }
        }
    }
    maxx = 0;
    maxi = -1;
    calc_possible_equip(c, 0);
    for (a = 0; a < tot; a++)
    {
        b = g_inv[t_inv[a]].item;
        if (items[b].stats[A_ATT] > maxx)
        {
            maxx = items[b].stats[A_ATT];
            maxi = a;
        }
    }
    if (maxi > -1)
    {
        if (equip(pidx[c], t_inv[maxi], 0) == 0)
        {
            return;
        }
    }
    for (z = 1; z < 5; z++)
    {
        maxx = 0;
        maxi = -1;
        calc_possible_equip(c, z);
        for (a = 0; a < tot; a++)
        {
            b = g_inv[t_inv[a]].item;
            if (items[b].stats[A_DEF] + items[b].stats[A_MAG] > maxx)
            {
                maxx = items[b].stats[A_DEF] + items[b].stats[A_MAG];
                maxi = a;
            }
        }
        if (maxi > -1)
        {
            if (equip(pidx[c], t_inv[maxi], 0) == 0)
            {
                return;
            }
        }
    }
    maxx = 0;
    maxi = -1;
    calc_possible_equip(c, 5);
    for (a = 0; a < tot; a++)
    {
        b = g_inv[t_inv[a]].item;
        for (z = 0; z < NUM_STATS; z++)
        {
            v += items[b].stats[z];
        }
        for (z = 0; z < R_TOTAL_RES; z++)
        {
            v += items[b].res[z];
        }
        if (v > maxx)
        {
            maxx = v;
            maxi = a;
        }
    }
    if (maxi > -1)
    {
        if (equip(pidx[c], t_inv[maxi], 0) == 0)
        {
            return;
        }
    }
    play_effect(SND_EQUIP, 128);
}
