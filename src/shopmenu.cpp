/*! \file
 * \brief Handles shops
 *
 * \author JB
 * \date ????????
 */

#include <cstdio>
#include <iomanip>
#include <string>
#include <sstream>

#include "constants.h"
#include "draw.h"
#include "gfx.h"
#include "input.h"
#include "itemdefs.h"
#include "itemmenu.h"
#include "kq.h"
#include "magic.h"
#include "music.h"
#include "res.h"
#include "setup.h"
#include "shopmenu.h"
#include "timing.h"

/* Winter Knight: I'm making it so shops are declared in scripts, rather than
in the code. It is part of my "separate the engine and the data" campaign. */

/* highest valid shops[] index + 1. Equals number of shops declared if indexes
    are declared in order. */
uint16_t num_shops = 0;

s_shop shops[NUMSHOPS]; /* Initialized by init.lua:init_shop() */

/*! Number of items in a shop */
static uint32_t num_shop_items;

/*  internal variables  */
/*! \brief Current shop index */
static uint8_t shop_no;

/*  internal functions  */
static void draw_sideshot(int);
static void buy_menu();
static void buy_item(int, int);
static void sell_menu();
static void sell_howmany(int, size_t);
static void sell_item(int, int);

/*! \brief Actually purchase the item
 *
 * This is used after selecting an item, from the above
 * menu, to determine who to give it to.  Then it gives
 * it to them and deducts the cash.
 *
 * \param   how_many Quantity
 * \param   item_no Index of item
 */
static void buy_item(int how_many, int item_no)
{
    int z = 0, l, stop = 0, cost;

    l = shops[shop_no].items[item_no];
    cost = items[l].price * how_many;
    if (cost > gp || how_many == 0)
    {
        play_effect(SND_BAD, 128);
        return;
    }
    while (!stop)
    {
        Game.do_check_animation();
        blit(back, double_buffer, 0, 0, xofs, 192 + yofs, KQ_SCREEN_W, 48);
        kqDraw.menubox(double_buffer, 32 + xofs, 168 + yofs, 30, 1, eMenuBoxColor::DARKBLUE);
        kqDraw.print_font(double_buffer, 104 + xofs, 176 + yofs, _("Confirm/Cancel"), eFontColor::FONTCOLOR_NORMAL);
        draw_sideshot(shops[shop_no].items[item_no]);
        kqDraw.blit2screen(xofs, yofs);

        PlayerInput.readcontrols();
        if (PlayerInput.balt)
        {
            Game.unpress();
            stop = 1;
        }
        if (PlayerInput.bctrl)
        {
            Game.unpress();
            return;
        }
    }
    z = check_inventory(l, how_many);
    if (z > 0)
    {
        gp = gp - cost;
        shops[shop_no].items_current[item_no] -= how_many;
        play_effect(SND_MONEY, 128);
        return;
    }
    play_effect(SND_BAD, 128);
    kqDraw.message(_("No room!"), -1, 0, xofs, yofs);
    return;
}

/*! \brief Show items to buy
 *
 * Show the player a list of items which can be bought
 * and wait for him/her to choose something or exit.
 */
static void buy_menu()
{
    bool stop = false;
    unsigned short xptr = 1, yptr = 0;
    unsigned int max_x = 0;
    if (num_shop_items < 1)
    {
        // This function should not be called with 0 shop items anyway, but
        // this silences an analysis warning.
        return;
    }
    for (unsigned int shop_item_index = 0; shop_item_index < num_shop_items; shop_item_index++)
    {
        if (shops[shop_no].items_current[shop_item_index] > max_x)
        {
            max_x = shops[shop_no].items_current[shop_item_index];
        }
    }

    if (max_x > 9)
    {
        max_x = 9;
    }
    while (!stop)
    {
        Game.do_check_animation();
        kqDraw.drawmap();
        kqDraw.menubox(double_buffer, 152 - (shop_name.length() * 4) + xofs, yofs, shop_name.length(), 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        kqDraw.print_font(double_buffer, 160 - (shop_name.length() * 4) + xofs, 8 + yofs, shop_name.c_str(), eFontColor::FONTCOLOR_GOLD);

        kqDraw.menubox(double_buffer, xofs, 208 + yofs, 7, 2, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        kqDraw.print_font(double_buffer, 24 + xofs, 220 + yofs, _("Buy"), eFontColor::FONTCOLOR_GOLD);

        kqDraw.menubox(double_buffer, 32 + xofs, 24 + yofs, 30, 16, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        kqDraw.menubox(double_buffer, 32 + xofs, 168 + yofs, 30, 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        draw_shopgold();
        for (unsigned int shop_item_index = 0; shop_item_index < num_shop_items; shop_item_index++)
        {
            unsigned short item_index = shops[shop_no].items[shop_item_index];
            unsigned short max = shops[shop_no].items_current[shop_item_index];
            if (xptr <= max)
            {
                max = xptr;
            }
            kqDraw.draw_icon(double_buffer, items[item_index].icon, 48 + xofs, shop_item_index * 8 + 32 + yofs);
            int cost = max * items[item_index].price;
            eFontColor font_color = cost <= gp ? eFontColor::FONTCOLOR_NORMAL : eFontColor::FONTCOLOR_DARK;
            kqDraw.print_font(double_buffer, 56 + xofs, shop_item_index * 8 + 32 + yofs, items[item_index].itemName, font_color);
            if (max > 1)
            {
                std::string maxStr{ "(" + std::to_string(max) + ")" };
                kqDraw.print_font(double_buffer, 256 + xofs, shop_item_index * 8 + 32 + yofs, maxStr.c_str(), font_color);
            }
            if (max > 0)
            {
                std::string castStr{ std::to_string(cost) };
                kqDraw.print_font(double_buffer, 248 - (castStr.length() * 8) + xofs, shop_item_index * 8 + 32 + yofs, castStr.c_str(), font_color);
            }
            else
            {
                kqDraw.print_font(double_buffer, 200 + xofs, shop_item_index * 8 + 32 + yofs, _("Sold Out!"), font_color);
            }
        }

        unsigned short item_no = shops[shop_no].items[yptr];
        kqDraw.print_font(double_buffer, 160 - (strlen(items[item_no].desc) * 4) + xofs, 176 + yofs, items[item_no].desc, eFontColor::FONTCOLOR_NORMAL);
        draw_sideshot(item_no);
        draw_sprite(double_buffer, menuptr, 32 + xofs, yptr * 8 + 32 + yofs);
        kqDraw.blit2screen(xofs, yofs);

        PlayerInput.readcontrols();
        if (PlayerInput.up)
        {
            Game.unpress();
            if (yptr > 0)
            {
                yptr--;
            }
            else
            {
                yptr = num_shop_items - 1;
            }
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.down)
        {
            Game.unpress();
            if (yptr < num_shop_items - 1)
            {
                yptr++;
            }
            else
            {
                yptr = 0;
            }
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.left && xptr > 1)
        {
            Game.unpress();
            xptr--;
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.right && xptr < max_x)
        {
            Game.unpress();
            xptr++;
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.balt)
        {
            Game.unpress();
            blit(double_buffer, back, xofs, 192 + yofs, 0, 0, 320, 48);
            unsigned short max = shops[shop_no].items_current[yptr];
            if (xptr < max)
            {
                max = xptr;
            }
            buy_item(max, yptr);
        }
        if (PlayerInput.bctrl)
        {
            Game.unpress();
            stop = true;
        }
    }
}

/*! \brief Restore characters according to Inn effects.
 *
 * This is separate so that these effects can be done from anywhere.
 *
 * \param   do_delay Whether or not to wait during the darkness...
 */
void do_inn_effects(int do_delay)
{
    for (size_t pidx_index = 0; pidx_index < numchrs; pidx_index++)
    {
        size_t party_index = activeAvatarIds[pidx_index];
        party[party_index].hp = party[party_index].mhp;
        party[party_index].mp = party[party_index].mmp;
        for (size_t stats_index = 0; stats_index < 8; stats_index++)
        {
            party[party_index].sts[stats_index] = 0;
        }
    }
    Music.pause_music();
    play_effect(36, 128);
    if (do_delay != 0)
    {
        kqDraw.do_transition(TRANS_FADE_OUT, 2);
        kqDraw.drawmap();
        kqDraw.blit2screen(xofs, yofs);
        kq_wait(1500);
        kqDraw.do_transition(TRANS_FADE_IN, 2);
    }
    save_spells[P_REPULSE] = 0;
    Music.resume_music();
}

/*! \brief Display amount of gold
 *
 * Display the party's funds.
 */
void draw_shopgold()
{
    kqDraw.menubox(double_buffer, 248 + xofs, 208 + yofs, 7, 2, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
    kqDraw.print_font(double_buffer, 256 + xofs, 216 + yofs, _("Gold:"), eFontColor::FONTCOLOR_GOLD);
    std::string gpStr{ std::to_string(gp) };
    kqDraw.print_font(double_buffer, 312 - (gpStr.length() * 8) + xofs, 224 + yofs, gpStr.c_str(), eFontColor::FONTCOLOR_NORMAL);
}

/*! \brief Show status info
 *
 * Well, it used to be on the side, but now it's on the bottom.
 * This displays the characters and whether or not they are
 * able to use/equip what is being looked at, and how it would
 * improve their stats (if applicable).
 *
 * \param   selected_item Item being looked at.
 */
void draw_sideshot(int selected_item)
{
    int wx, wy;
    int cs[13];
    uint32_t ownd = 0, equipped_items = 0, slot;
    size_t pidx_index, equipment_index, stats_index, cs_index, spell_index, inventory_index;

    kqDraw.menubox(double_buffer, 80 + xofs, 192 + yofs, 18, 4, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
    for (pidx_index = 0; pidx_index < numchrs; pidx_index++)
    {
        wx = pidx_index * 72 + 88 + xofs;
        wy = 200 + yofs;
        draw_sprite(double_buffer, frames[activeAvatarIds[pidx_index]][2], wx, wy);
    }
    if (selected_item == -1)
    {
        return;
    }
    slot = items[selected_item].type;
    for (pidx_index = 0; pidx_index < numchrs; pidx_index++)
    {
        wx = pidx_index * 72 + 88 + xofs;
        wy = 200 + yofs;
        for (equipment_index = 0; equipment_index < NUM_EQUIPMENT; equipment_index++)
        {
            if (party[activeAvatarIds[pidx_index]].eqp[equipment_index] == selected_item)
            {
                equipped_items++;
            }
        }
        if (slot < 6)
        {
            if (party[activeAvatarIds[pidx_index]].eqp[slot] > 0)
            {
                for (stats_index = 0; stats_index < NUM_STATS; stats_index++)
                {
                    cs[stats_index] = items[selected_item].stats[stats_index] - items[party[activeAvatarIds[pidx_index]].eqp[slot]].stats[stats_index];
                }
            }
            else
            {
                for (stats_index = 0; stats_index < NUM_STATS; stats_index++)
                {
                    cs[stats_index] = items[selected_item].stats[stats_index];
                }
            }
            if (slot == 0)
            {
                kqDraw.draw_icon(double_buffer, 3, wx + 16, wy);
                kqDraw.print_font(double_buffer, wx + 16, wy + 8, "%", eFontColor::FONTCOLOR_NORMAL);
                for (cs_index = 0; cs_index < 2; cs_index++)
                {
                    if (cs[cs_index + 8] < 0)
                    {
                        std::stringstream str;
                        str << std::setw(4) << cs[cs_index + 8];
                        //sprintf(strbuf, "%-4d", cs[cs_index + 8]);
                        kqDraw.print_font(double_buffer, wx + 24, cs_index * 8 + wy, str.str().c_str(), eFontColor::FONTCOLOR_RED);
                    }
                    else if (cs[cs_index + 8] > 0)
                    {
                        std::stringstream str;
                        str << "+" << std::setw(3) << cs[cs_index + 8];
                        //sprintf(strbuf, "+%-3d", cs[cs_index + 8]);
                        kqDraw.print_font(double_buffer, wx + 24, cs_index * 8 + wy, str.str().c_str(), eFontColor::FONTCOLOR_GREEN);
                    }
                    else if (cs[cs_index + 8] == 0)
                    {
                        kqDraw.print_font(double_buffer, wx + 24, cs_index * 8 + wy, "=", eFontColor::FONTCOLOR_NORMAL);
                    }
                }
            }
            else
            {
                kqDraw.draw_icon(double_buffer, 9, wx + 16, wy);
                kqDraw.print_font(double_buffer, wx + 16, wy + 8, "%", eFontColor::FONTCOLOR_NORMAL);
                kqDraw.draw_icon(double_buffer, 47, wx + 16, wy + 16);
                for (cs_index = 0; cs_index < 3; cs_index++)
                {
                    if (cs[cs_index + 10] < 0)
                    {
                        std::stringstream str;
                        str << std::setw(4) << cs[cs_index + 8];
                        //sprintf(strbuf, "%-4d", cs[cs_index + 10]);
                        kqDraw.print_font(double_buffer, wx + 24, cs_index * 8 + wy, str.str().c_str(), eFontColor::FONTCOLOR_RED);
                    }
                    else if (cs[cs_index + 10] > 0)
                    {
                        std::stringstream str;
                        str << "+" << std::setw(3) << cs[cs_index + 8];
                        //sprintf(strbuf, "+%-3d", cs[cs_index + 10]);
                        kqDraw.print_font(double_buffer, wx + 24, cs_index * 8 + wy, str.str().c_str(), eFontColor::FONTCOLOR_GREEN);
                    }
                    else if (cs[cs_index + 10] == 0)
                    {
                        kqDraw.print_font(double_buffer, wx + 24, cs_index * 8 + wy, "=", eFontColor::FONTCOLOR_NORMAL);
                    }
                }
            }
            if (items[selected_item].eq[activeAvatarIds[pidx_index]] == 0)
            {
                draw_sprite(double_buffer, noway, wx, wy);
            }
        }
        else
        {
            if (items[selected_item].icon == W_SBOOK || items[selected_item].icon == W_ABOOK)
            {
                for (spell_index = 0; spell_index < 60; spell_index++)
                {
                    if (party[activeAvatarIds[pidx_index]].spells[spell_index] == items[selected_item].hnds)
                    {
                        draw_sprite(double_buffer, noway, wx, wy);
                    }
                }
            }
        }
    }
    for (inventory_index = 0; inventory_index < MAX_INV; inventory_index++)
    {
        if (g_inv[inventory_index].item == selected_item)
        {
            ownd += g_inv[inventory_index].quantity; // quantity of this item
        }
    }
    std::string owndStr = "Own: " + std::to_string(ownd);
    kqDraw.print_font(double_buffer, 88 + xofs, 224 + yofs, _(owndStr.c_str()), eFontColor::FONTCOLOR_NORMAL);
    if (slot < 6)
    {
        std::string equippedItemsStr = "Eqp: " + std::to_string(equipped_items);
        kqDraw.print_font(double_buffer, 160 + xofs, 224 + yofs, _(equippedItemsStr.c_str()), eFontColor::FONTCOLOR_NORMAL);
    }
}

/*! \brief Handle Inn functions
 *
 * This is simply used for staying at the inn.  Remember
 * it costs more money to stay if your characters require
 * healing or resurrection.
 *
 * \param   iname Name of Inn
 * \param   gold_per_character Gold per character (base price)
 * \param   pay If 0, staying is free.
 */
void inn(const char* iname, uint32_t gold_per_character, int pay)
{
    int b, my = 0, stop = 0;
    uint32_t total_gold_cost;
    size_t pidx_index, party_index;

    if (pay == 0)
    {
        /* TT add: (pay) is also used now to indicate whether we should wait
         *         (fade in/out) or just heal the heroes and be done
         */
        do_inn_effects(pay);
        return;
    }
    Game.unpress();
    kqDraw.drawmap();
    kqDraw.menubox(double_buffer, 152 - (strlen(iname) * 4) + xofs, yofs, strlen(iname), 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
    kqDraw.print_font(double_buffer, 160 - (strlen(iname) * 4) + xofs, 8 + yofs, iname, eFontColor::FONTCOLOR_GOLD);
    total_gold_cost = gold_per_character;
    for (party_index = 0; party_index < numchrs; party_index++)
    {
        pidx_index = activeAvatarIds[party_index];
        if (party[pidx_index].sts[S_POISON] != 0)
        {
            total_gold_cost += gold_per_character / 2;
        }
        if (party[pidx_index].sts[S_BLIND] != 0)
        {
            total_gold_cost += gold_per_character / 2;
        }
        if (party[pidx_index].sts[S_MUTE] != 0)
        {
            total_gold_cost += gold_per_character / 2;
        }
        if (party[pidx_index].sts[S_DEAD] != 0)
        {
            b = gold_per_character / 2;
            total_gold_cost += (b * party[pidx_index].lvl / 5);
        }
    }
    while (!stop)
    {
        Game.do_check_animation();
        kqDraw.drawmap();

        std::string totalGoldCostStr = "The cost is " + std::to_string(total_gold_cost) + " gp for the night.";
        kqDraw.menubox(double_buffer, 152 - (totalGoldCostStr.length() * 4) + xofs, 48 + yofs, totalGoldCostStr.length(), 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        kqDraw.print_font(double_buffer, 160 - (totalGoldCostStr.length() * 4) + xofs, 56 + yofs, _(totalGoldCostStr.c_str()), eFontColor::FONTCOLOR_NORMAL);
        kqDraw.menubox(double_buffer, 248 + xofs, 168 + yofs, 7, 2, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        kqDraw.print_font(double_buffer, 256 + xofs, 176 + yofs, _("Gold:"), eFontColor::FONTCOLOR_GOLD);
        std::string gpStr = std::to_string(gp);
        kqDraw.print_font(double_buffer, 312 - (gpStr.length() * 8) + xofs, 184 + yofs, gpStr.c_str(), eFontColor::FONTCOLOR_NORMAL);
        if ((uint32_t)gp >= total_gold_cost)
        {
            kqDraw.menubox(double_buffer, 52 + xofs, 96 + yofs, 25, 2, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
            kqDraw.print_font(double_buffer, 60 + xofs, 108 + yofs, _("Do you wish to stay?"), eFontColor::FONTCOLOR_NORMAL);
        }
        else
        {
            kqDraw.menubox(double_buffer, 32 + xofs, 96 + yofs, 30, 2, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
            kqDraw.print_font(double_buffer, 40 + xofs, 108 + yofs, _("You can't afford to stay here."), eFontColor::FONTCOLOR_NORMAL);
            kqDraw.blit2screen(xofs, yofs);
            Game.wait_enter();
            return;
        }

        kqDraw.menubox(double_buffer, 220 + xofs, 96 + yofs, 4, 2, eMenuBoxColor::DARKBLUE);
        kqDraw.print_font(double_buffer, 236 + xofs, 104 + yofs, _("yes"), eFontColor::FONTCOLOR_NORMAL);
        kqDraw.print_font(double_buffer, 236 + xofs, 112 + yofs, _("no"), eFontColor::FONTCOLOR_NORMAL);
        draw_sprite(double_buffer, menuptr, 220 + xofs, my * 8 + 104 + yofs);
        kqDraw.blit2screen(xofs, yofs);
        PlayerInput.readcontrols();
        if (PlayerInput.down)
        {
            Game.unpress();
            if (my == 0)
            {
                my = 1;
            }
            else
            {
                my = 0;
            }
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.up)
        {
            Game.unpress();
            if (my == 0)
            {
                my = 1;
            }
            else
            {
                my = 0;
            }
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.balt)
        {
            Game.unpress();
            if (my == 0)
            {
                gp -= total_gold_cost;
                do_inn_effects(pay);
                stop = 1;
            }
            else
            {
                stop = 2;
            }
        }
    }
    timer_count = 0;
}

/*! \brief Ask player the quantity to sell
 *
 * Inquire as to what quantity of the current item, the
 * character wishes to sell.
 *
 * \param   item_no Index of item in inventory
 * \param   inv_page Page of the inventory
 */
static void sell_howmany(int item_no, size_t inv_page)
{
    int l, max_items, prc, my = 1, stop;

    stop = 0;
    l = g_inv[inv_page * NUM_ITEMS_PER_PAGE + item_no].item;
    prc = items[l].price;
    if (prc == 0)
    {
        play_effect(SND_BAD, 128);
        return;
    }
    // Maximum (total) number of items
    max_items = g_inv[inv_page * NUM_ITEMS_PER_PAGE + item_no].quantity;
    if (max_items == 1)
    {
        kqDraw.menubox(double_buffer, 32 + xofs, 168 + yofs, 30, 1, eMenuBoxColor::DARKBLUE);
        std::string prcStr = "Sell for " + std::to_string(prc * 50 / 100) + " gp?";
        kqDraw.print_font(double_buffer, 160 - (prcStr.length() * 4) + xofs, 176 + yofs, _(prcStr.c_str()), eFontColor::FONTCOLOR_NORMAL);
        sell_item(inv_page * NUM_ITEMS_PER_PAGE + item_no, 1);
        stop = 1;
    }
    while (!stop)
    {
        Game.do_check_animation();
        kqDraw.drawmap();
        kqDraw.menubox(double_buffer, 32 + xofs, 168 + yofs, 30, 1, eMenuBoxColor::DARKBLUE);
        kqDraw.print_font(double_buffer, 124 + xofs, 176 + yofs, _("How many?"), eFontColor::FONTCOLOR_NORMAL);
        kqDraw.menubox(double_buffer, 32 + xofs, item_no * 8 + 24 + yofs, 30, 1, eMenuBoxColor::DARKBLUE);
        kqDraw.draw_icon(double_buffer, items[l].icon, 48 + xofs, item_no * 8 + 32 + yofs);
        kqDraw.print_font(double_buffer, 56 + xofs, item_no * 8 + 32 + yofs, items[l].itemName, eFontColor::FONTCOLOR_NORMAL);
        std::string itemsStr = std::to_string(my) + " of " + std::to_string(max_items);
        kqDraw.print_font(double_buffer, 280 - (itemsStr.length() * 8) + xofs, item_no * 8 + 32 + yofs, _(itemsStr.c_str()), eFontColor::FONTCOLOR_NORMAL);
        kqDraw.blit2screen(xofs, yofs);

        PlayerInput.readcontrols();
        if (PlayerInput.up || PlayerInput.right)
        {
            if (my < max_items)
            {
                Game.unpress();
                my++;
            }
            else
            {
                Game.unpress();
                my = 1;
            }
        }
        if (PlayerInput.down || PlayerInput.left)
        {
            if (my > 1)
            {
                Game.unpress();
                my--;
            }
            else
            {
                Game.unpress();
                my = max_items;
            }
        }
        if (PlayerInput.balt)
        {
            Game.unpress();
            kqDraw.menubox(double_buffer, 32 + xofs, 168 + yofs, 30, 1, eMenuBoxColor::DARKBLUE);
            std::string prcStr = "Sell for " + std::to_string((prc * 50 / 100) * my) + " gp?";
            kqDraw.print_font(double_buffer, 160 - (prcStr.length() * 4) + xofs, 176 + yofs, _(prcStr.c_str()), eFontColor::FONTCOLOR_NORMAL);
            sell_item(inv_page * NUM_ITEMS_PER_PAGE + item_no, my);
            stop = 1;
        }
        if (PlayerInput.bctrl)
        {
            Game.unpress();
            stop = 1;
        }
    }
}

/*! \brief Actually sell item
 *
 * Confirm the price of the sale with the player, and then
 * complete the transaction.
 *
 * \param   itno Index of item
 * \param   ni Quantity being sold
 */
static void sell_item(int itno, int ni)
{
    int l, stop = 0, sp, a;

    l = g_inv[itno].item;
    sp = (items[l].price * 50 / 100) * ni;
    kqDraw.menubox(double_buffer, 96 + xofs, 192 + yofs, 14, 1, eMenuBoxColor::DARKBLUE);
    kqDraw.print_font(double_buffer, 104 + xofs, 200 + yofs, _("Confirm/Cancel"), eFontColor::FONTCOLOR_NORMAL);
    kqDraw.blit2screen(xofs, yofs);
    while (!stop)
    {
        PlayerInput.readcontrols();
        if (PlayerInput.balt)
        {
            Game.unpress();
            gp += sp;
            for (a = 0; a < SHOPITEMS; a++)
            {
                if (l > 0 && shops[shop_no].items[a] == l)
                {
                    shops[shop_no].items_current[a] += ni;
                    if (shops[shop_no].items_current[a] > shops[shop_no].items_max[a])
                    {
                        shops[shop_no].items_current[a] = shops[shop_no].items_max[a];
                    }
                }
            }
            play_effect(SND_MONEY, 128);
            remove_item(itno, ni);
            stop = 1;
        }
        if (PlayerInput.bctrl)
        {
            Game.unpress();
            stop = 1;
        }
    }
}

/*! \brief Show items that can be sold
 *
 * Display a list of items that are in inventory and ask which
 * item or items to sell.
 */
static void sell_menu()
{
    size_t yptr = 0, stop = 0;
    int p, sp;
    eFontColor k;
    size_t inv_page = 0;

    while (!stop)
    {
        Game.do_check_animation();
        kqDraw.drawmap();
        kqDraw.menubox(double_buffer, 152 - (shop_name.length() * 4) + xofs, yofs, shop_name.length(), 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        kqDraw.print_font(double_buffer, 160 - (shop_name.length() * 4) + xofs, 8 + yofs, shop_name.c_str(), eFontColor::FONTCOLOR_GOLD);
        kqDraw.menubox(double_buffer, xofs, 208 + yofs, 7, 2, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        kqDraw.print_font(double_buffer, 20 + xofs, 220 + yofs, _("Sell"), eFontColor::FONTCOLOR_GOLD);
        kqDraw.menubox(double_buffer, 32 + xofs, 24 + yofs, 30, 16, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        kqDraw.menubox(double_buffer, 32 + xofs, 168 + yofs, 30, 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        draw_shopgold();
        for (p = 0; p < NUM_ITEMS_PER_PAGE; p++)
        {
            int inventory_item_index = g_inv[inv_page * NUM_ITEMS_PER_PAGE + p].item;
            if (items[inventory_item_index].price == 0)
            {
                k = eFontColor::FONTCOLOR_DARK;
            }
            else
            {
                k = eFontColor::FONTCOLOR_NORMAL;
            }
            kqDraw.draw_icon(double_buffer, items[inventory_item_index].icon, 48 + xofs, p * 8 + 32 + yofs);
            kqDraw.print_font(double_buffer, 56 + xofs, p * 8 + 32 + yofs, items[inventory_item_index].itemName, k);
            // Check if quantity of this item > 1
            auto quantity = g_inv[inv_page * NUM_ITEMS_PER_PAGE + p].quantity;
            if (quantity > 1)
            {
                // The '^' in this is an 'x' in allfonts.pcx
                std::string qtyStr = "^" + std::to_string(quantity);
                kqDraw.print_font(double_buffer, 264 + xofs, p * 8 + 32 + yofs, qtyStr.c_str(), k);
            }
        }
        s_inventory& inv = g_inv[inv_page * NUM_ITEMS_PER_PAGE + yptr];
        sp = items[inv.item].price * 50 / 100;
        if (items[inv.item].price > 0)
        {
            std::string qtyStr = (inv.quantity > 1)
                ? std::to_string(sp) + " gp for each one."
                : "That's worth " + std::to_string(sp) + " gp.";
            kqDraw.print_font(double_buffer, 160 - (qtyStr.length() * 4) + xofs, 176 + yofs, _(qtyStr.c_str()), eFontColor::FONTCOLOR_NORMAL);
        }
        else
        {
            if (inv.item > 0)
            {
                kqDraw.print_font(double_buffer, 76 + xofs, 192 + yofs, _("That cannot be sold!"), eFontColor::FONTCOLOR_NORMAL);
            }
        }
        draw_sprite(double_buffer, menuptr, 32 + xofs, yptr * 8 + 32 + yofs);
        draw_sprite(double_buffer, pgb[inv_page], 278 + xofs, 158 + yofs);
        kqDraw.blit2screen(xofs, yofs);

        PlayerInput.readcontrols();

        if (PlayerInput.down)
        {
            Game.unpress();
            if (yptr < (NUM_ITEMS_PER_PAGE - 1))
            {
                yptr++;
            }
            else
            {
                yptr = 0;
            }
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.up)
        {
            Game.unpress();
            if (yptr > 0)
            {
                yptr--;
            }
            else
            {
                yptr = (NUM_ITEMS_PER_PAGE - 1);
            }
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.left)
        {
            Game.unpress();
            if (inv_page > 0)
            {
                inv_page--;
            }
            else
            {
                inv_page = MAX_INV / NUM_ITEMS_PER_PAGE - 1;
            }
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.right)
        {
            Game.unpress();
            if (inv_page < (MAX_INV / NUM_ITEMS_PER_PAGE - 1))
            {
                inv_page++;
            }
            else
            {
                inv_page = 0;
            }
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.balt)
        {
            Game.unpress();
            if (g_inv[inv_page * NUM_ITEMS_PER_PAGE + yptr].item > 0 && items[g_inv[inv_page * NUM_ITEMS_PER_PAGE + yptr].item].price > 0)
            {
                sell_howmany(yptr, inv_page);
            }
        }
        if (PlayerInput.bctrl)
        {
            Game.unpress();
            stop = 1;
        }
    }
}

/*! \brief Main entry point to shop functions
 *
 * The initial shop dialog.  This function calculates item quantities
 * and then just asks if we're buying or selling.
 *
 * \param   shop_num Index of this shop
 * \returns 1 if shop has no items, 0 otherwise
 */
int shop(int shop_num)
{
    int ptr = 0;
    bool stop = false;
    s_shop& shop = shops[shop_num];
    shop_no = shop_num;
    shop_name = shop.shopName;

    /* If enough time has passed, fully replenish this shop's stock of an item */
    int replenish_time = (khr * 60) + kmin - shop.time;
    bool first_visit = (shop.time == 0);
    for (int a = 0; a < SHOPITEMS; a++)
    {
        if (shop.items_replenish_time[a] > 0)
        {
            if (first_visit || replenish_time > shop.items_replenish_time[a])
            {
                shop.items_current[a] = shop.items_max[a];
            }
        }
    }

    /* Return 1 if shop has no items to sell */
    num_shop_items = SHOPITEMS - 1;
    for (int a = SHOPITEMS; a > 0; a--)
    {
        if (shop.items[a - 1] == 0)
        {
            num_shop_items = a - 1;
        }
    }
    if (num_shop_items == 0)
    {
        return 1;
    }

    Game.unpress();
    play_effect(SND_MENU, 128);
    while (!stop)
    {
        Game.do_check_animation();
        kqDraw.drawmap();
        kqDraw.menubox(double_buffer, 152 - (shop_name.length() * 4) + xofs, yofs, shop_name.length(), 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        kqDraw.print_font(double_buffer, 160 - (shop_name.length() * 4) + xofs, 8 + yofs, shop_name.c_str(), eFontColor::FONTCOLOR_GOLD);
        kqDraw.menubox(double_buffer, 32 + xofs, 24 + yofs, 30, 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        kqDraw.menubox(double_buffer, ptr * 80 + 32 + xofs, 24 + yofs, 10, 1, eMenuBoxColor::DARKBLUE);
        kqDraw.print_font(double_buffer, 68 + xofs, 32 + yofs, _("Buy"), eFontColor::FONTCOLOR_GOLD);
        kqDraw.print_font(double_buffer, 144 + xofs, 32 + yofs, _("Sell"), eFontColor::FONTCOLOR_GOLD);
        kqDraw.print_font(double_buffer, 224 + xofs, 32 + yofs, _("Exit"), eFontColor::FONTCOLOR_GOLD);
        draw_sideshot(-1);
        draw_shopgold();
        kqDraw.blit2screen(xofs, yofs);

        PlayerInput.readcontrols();

        if (PlayerInput.left && ptr > 0)
        {
            Game.unpress();
            ptr--;
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.right && ptr < 2)
        {
            Game.unpress();
            ptr++;
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.balt)
        {
            Game.unpress();
            if (ptr == 0)
            {
                buy_menu();
            }
            if (ptr == 1)
            {
                sell_menu();
            }
            if (ptr == 2)
            {
                stop = true;
            }
        }
        if (PlayerInput.bctrl)
        {
            Game.unpress();
            stop = true;
        }
    }
    shop.time = khr * 60 + kmin;
    return 0;
}
