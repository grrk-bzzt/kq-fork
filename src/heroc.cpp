/*! \file
 * \brief   Hero combat
 * \author  Josh Bolduc
 * \date    ????????
 *
 * Stuff relating to hero's special combat skills
 *
 * \todo PH Make sure we understand the two methods of referring to a
 *          hero - either as an index in the pidx array or an index in
 *          the party array
 */

#include "heroc.h"
#include "combat.h"
#include "draw.h"
#include "effects.h"
#include "eqpmenu.h"
#include "gfx.h"
#include "hskill.h"
#include "imgcache.h"
#include "input.h"
#include "itemdefs.h"
#include "itemmenu.h"
#include "kq.h"
#include "magic.h"
#include "menu.h"
#include "platform.h"
#include "random.h"
#include "res.h"
#include "selector.h"
#include "setup.h"
#include "skills.h"
#include "timing.h"

#include <memory>
#include <stdio.h>
#include <string.h>

KHero heroc;

KHero::KHero()
{
}

KHero::~KHero()
{
}

void KHero::auto_herochooseact(int who)
{
    int eact;

    if (!kqCombat.bIsEtherEffectActive[who])
    {
        return;
    }
    if (fighter[who].fighterSpellEffectStats[S_DEAD] == eDeathType::IS_DEAD || fighter[who].fighterHealth <= 0)
    {
        kqCombat.bIsEtherEffectActive[who] = false;
        return;
    }
    fighter[who].fighterSpriteFacing = 0;
    eact = kqrandom->random_range_exclusive(0, 4);
    if (eact == 0)
    {
        kqCombat.bIsEtherEffectActive[who] = false;
        return;
    }
    if (eact == 1)
    {
        fighter[who].ctmem = 0;
        hero_attack(who);
        kqCombat.bIsEtherEffectActive[who] = false;
        return;
    }
    fighter[who].ctmem = auto_select_hero(who, 0);
    hero_attack(who);
    kqCombat.bIsEtherEffectActive[who] = false;
}

int KHero::available_spells(int who)
{
    int a, b, e, l, numsp = 0;

    l = pidx[who];
    for (a = 0; a < 60; a++)
    {
        b = party[l].spells[a];
        if (b > 0)
        {
            if (magic[b].use == USE_ANY_INF || magic[b].use == USE_COMBAT_INF)
            {
                if (l == CORIN && fighter[who].aux == 2)
                {
                    e = mp_needed(who, b);
                    if (fighter[who].fighterMagic >= e && magic[b].elem < 9)
                    {
                        numsp++;
                    }
                }
                else
                {
                    e = mp_needed(who, b);
                    if (fighter[who].fighterMagic >= e)
                    {
                        numsp++;
                    }
                }
            }
        }
    }
    return numsp;
}

int KHero::can_invoke_item(int t1)
{
    if (items[t1].type > 5)
    {
        return 0;
    }
    if (items[t1].use != USE_COMBAT_ONCE && items[t1].use != USE_COMBAT_INF)
    {
        return 0;
    }
    return 1;
}

int KHero::combat_castable(int spell_caster, int spell_number)
{
    int b, c = 0;

    b = party[pidx[spell_caster]].spells[spell_number];
    if (b == M_WARP)
    {
#ifdef DEBUGMODE
        // They can only run if we are in debugging mode >= 3
        if (can_run == 0 && debugging < 3)
        {
            return 0;
        }
#else
        if (can_run == 0)
        {
            return 0;
        }
#endif
    }

    if (magic[b].use == USE_ANY_INF || magic[b].use == USE_COMBAT_INF)
    {
        if (pidx[spell_caster] == CORIN && fighter[c].aux == 2)
        {
            c = mp_needed(spell_caster, b);
            if (fighter[spell_caster].fighterMagic >= c && magic[b].elem < R_BLIND)
            {
                return 1;
            }
        }
        else
        {
            c = mp_needed(spell_caster, b);
            if (fighter[spell_caster].fighterMagic >= c)
            {
                return 1;
            }
        }
    }
    return 0;
}

void KHero::allowItemUseDuringBattle(bool canUseItem)
{
    m_CanUseItem = canUseItem;
}

bool KHero::canItemBeUsedDuringBattle() const
{
    return m_CanUseItem;
}

void KHero::combat_draw_items(int pg)
{
    int a, b, c;
    eFontColor k;

    kqDraw.menubox(double_buffer, 72, 8, 20, 16, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
    for (a = 0; a < 16; a++)
    {
        // b == item index #
        b = g_inv[pg * 16 + a].item;
        // c == quantity of item
        c = g_inv[pg * 16 + a].quantity;
        kqDraw.draw_icon(double_buffer, items[b].icon, 88, a * 8 + 16);
        if (combat_item_usable(b) == 1)
        {
            k = eFontColor::FONTCOLOR_NORMAL;
        }
        else
        {
            k = eFontColor::FONTCOLOR_DARK;
        }
        kqDraw.print_font(double_buffer, 96, a * 8 + 16, items[b].itemName, k);
        if (c > 1)
        {
            sprintf(strbuf, "^%d", c);
            kqDraw.print_font(double_buffer, 224, a * 8 + 16, strbuf, k);
        }
    }
    draw_sprite(double_buffer, pgb[pg], 238, 142);
}

void KHero::combat_draw_spell_menu(int c, int ptr, int pg)
{
    int z, j, b;

    kqDraw.menubox(double_buffer, 80, 24, 18, 12, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
    for (j = 0; j < NUM_SPELLS_PER_PAGE; j++)
    {
        z = party[pidx[c]].spells[pg * NUM_SPELLS_PER_PAGE + j];
        if (z > 0)
        {
            kqDraw.draw_icon(double_buffer, magic[z].icon, 96, j * 8 + 32);
            if (combat_castable(c, pg * NUM_SPELLS_PER_PAGE + j) == 1)
            {
                kqDraw.print_font(double_buffer, 104, j * 8 + 32, magic[z].spellName, eFontColor::FONTCOLOR_NORMAL);
            }
            else
            {
                kqDraw.print_font(double_buffer, 104, j * 8 + 32, magic[z].spellName, eFontColor::FONTCOLOR_DARK);
            }
            b = mp_needed(c, z);
            sprintf(strbuf, "%d", b);
            kqDraw.print_font(double_buffer, 222 - (strlen(strbuf) * 8), j * 8 + 32, strbuf, eFontColor::FONTCOLOR_NORMAL);
            draw_sprite(double_buffer, b_mp, 222, j * 8 + 32);
        }
    }
    draw_sprite(double_buffer, menuptr, 80, ptr * 8 + 32);
    draw_sprite(double_buffer, pgb[pg], 230, 126);
}

int KHero::combat_item(int ss, int t1, int tg)
{
    int a, b, r, st = tg, tt = 0, tl = 0;

    // Item cannot be used in combat
    if (combat_item_usable(t1) == 0)
    {
        return 0;
    }
    strcpy(attack_string, items[t1].itemName);
    display_attack_string = true;
    r = item_effects(ss, tg, t1);
    display_attack_string = false;
    if (r < 2)
    {
        return r;
    }
    if (items[t1].tgt == TGT_ENEMY_ALL)
    {
        tl = 1;
        if (ss == MAX_PARTY_SIZE)
        {
            st = 0;
            tt = numchrs;
        }
        else
        {
            st = MAX_PARTY_SIZE;
            tt = kqCombat.num_enemies;
        }
    }
    display_amount(st, eFont::FONT_DECIDE, tl);
    for (a = st; a < st + tt; a++)
    {
        adjust_hp(a, kqCombat.ta[a]);
    }
    b = 0;
    for (a = st; a < st + tt; a++)
    {
        if (fighter[a].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD && fighter[a].fighterHealth <= 0)
        {
            kqCombat.fkill(a);
            b++;
        }
        else
        {
            kqCombat.ta[a] = 0;
        }
    }
    if (b > 0)
    {
        death_animation(st, tl);
    }
    return 1;
}

int KHero::combat_item_menu(int whom)
{
    int z, stop = 0, ptr = 0, pptr = 0;

    fullblit(double_buffer, back);
    while (!stop)
    {
        Game.do_check_animation();
        fullblit(back, double_buffer);
        combat_draw_items(pptr);
        draw_sprite(double_buffer, menuptr, 72, ptr * 8 + 16);
        /* put description of selected item */
        kqDraw.menubox(double_buffer, 72, 152, 20, 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        kqDraw.print_font(double_buffer, 80, 160, items[g_inv[ptr + pptr * 16].item].desc, eFontColor::FONTCOLOR_NORMAL);
        kqDraw.blit2screen(0, 0);

        PlayerInput.readcontrols();
        if (PlayerInput.up)
        {
            Game.unpress();
            ptr--;
            if (ptr < 0)
            {
                ptr = 15;
            }
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.down)
        {
            Game.unpress();
            ptr++;
            if (ptr > 15)
            {
                ptr = 0;
            }
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.left)
        {
            Game.unpress();
            pptr--;
            if (pptr < 0)
            {
                pptr = 3;
            }
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.right)
        {
            Game.unpress();
            pptr++;
            if (pptr > 3)
            {
                pptr = 0;
            }
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.balt)
        {
            unsigned short inventory = g_inv[pptr * 16 + ptr].item;
            Game.unpress();
            if (items[inventory].tgt >= TGT_ENEMY_ONE)
            {
                z = select_enemy(whom, (eTarget)(items[inventory].tgt));
            }
            else
            {
                if (inventory == I_LTONIC)
                {
                    z = select_hero(whom, (eTarget)(items[inventory].tgt), 1);
                }
                else
                {
                    z = select_hero(whom, (eTarget)(items[inventory].tgt), 0);
                }
            }
            if (z > -1)
            {
                if (combat_item(0, inventory, z) == 1)
                {
                    if (items[fighter[whom].csmem].use != USE_ANY_INF &&
                        items[fighter[whom].csmem].use != USE_COMBAT_INF)
                    {
                        remove_item(pptr * 16 + ptr, 1);
                    }
                    stop = 2;
                }
            }
        }
        if (PlayerInput.bctrl)
        {
            Game.unpress();
            stop = 1;
        }
    }
    return stop - 1;
}

int KHero::combat_item_usable(int itno)
{
    // FIXME: What is this magic number '6'?
    if (items[itno].type != 6 || items[itno].tgt == TGT_NONE)
    {
        return 0;
    }
    if (items[itno].use == USE_NOT || items[itno].use == USE_CAMP_ONCE || items[itno].use == USE_CAMP_INF)
    {
        return 0;
    }
    return 1;
}

int KHero::combat_spell_menu(int c)
{
    int ptr = 0, pgno = 0, stop = 0;

    fullblit(double_buffer, back);
    play_effect(SND_MENU, 128);
    while (!stop)
    {
        Game.do_check_animation();
        fullblit(back, double_buffer);
        combat_draw_spell_menu(c, ptr, pgno);
        kqDraw.blit2screen(0, 0);

        PlayerInput.readcontrols();

        if (PlayerInput.down)
        {
            Game.unpress();
            ptr++;
            if (ptr > 11)
            {
                ptr = 0;
            }
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.up)
        {
            Game.unpress();
            ptr--;
            if (ptr < 0)
            {
                ptr = 11;
            }
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.right)
        {
            Game.unpress();
            pgno++;
            if (pgno > 4)
            {
                pgno = 0;
            }
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.left)
        {
            Game.unpress();
            pgno--;
            if (pgno < 0)
            {
                pgno = 4;
            }
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.balt)
        {
            Game.unpress();
            if (combat_castable(c, pgno * NUM_SPELLS_PER_PAGE + ptr) == 1)
            {
                fighter[c].csmem = party[pidx[c]].spells[pgno * NUM_SPELLS_PER_PAGE + ptr];
                stop = 2;
            }
        }
        if (PlayerInput.bctrl)
        {
            Game.unpress();
            stop = 1;
        }
    }
    if (stop == 2)
    {
        if ((fighter[c].csmem == M_LIFE || fighter[c].csmem == M_FULLLIFE) && numchrs == 1)
        {
            return 0;
        }
        if (pidx[c] == CORIN && fighter[c].aux == 2)
        {
            return 1;
        }
        if (combat_spell_targeting(c) == 1)
        {
            if (combat_spell(c, 0) > -1)
            {
                return 1;
            }
        }
    }
    return 0;
}

int KHero::combat_spell_targeting(int whom)
{
    int a, tg;

    a = fighter[whom].csmem;
    if (magic[a].tgt == 0)
    {
        return -1;
    }
    if (magic[a].tgt <= TGT_ALLY_ALL)
    {
        if (a == M_LIFE || a == M_FULLLIFE)
        {
            tg = select_hero(whom, (eTarget)(magic[a].tgt), NO_STS_CHECK);
        }
        else
        {
            tg = select_hero(whom, (eTarget)(magic[a].tgt), 0);
        }
        if (tg == -1)
        {
            return 0;
        }
        else
        {
            fighter[whom].ctmem = tg;
        }
    }
    else
    {
        tg = select_enemy(whom, (eTarget)(magic[a].tgt));
        if (tg == -1)
        {
            return 0;
        }
        else
        {
            fighter[whom].ctmem = tg;
        }
    }
    return 1;
}

void KHero::draw_invokable(int dud)
{
    int a, tt;
    eFontColor grd;

    kqDraw.menubox(double_buffer, 72, 80, 20, 6, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
    for (a = 0; a < NUM_EQUIPMENT; a++)
    {
        tt = party[dud].eqp[a];
        grd = can_invoke_item(tt) ? eFontColor::FONTCOLOR_NORMAL : eFontColor::FONTCOLOR_DARK;
        kqDraw.draw_icon(double_buffer, items[tt].icon, 88, a * 8 + 88);
        kqDraw.print_font(double_buffer, 96, a * 8 + 88, items[tt].itemName, grd);
    }
}

int KHero::hero_attack(int whom)
{
    int tgt;

    if (fighter[whom].fighterSpellEffectStats[S_CHARM] == 0)
    {
        tgt = select_enemy(whom, TGT_ENEMY_ONE);
    }
    else
    {
        /* PH fixme: replaced 99 with NO_STS_CHECK */
        /* was 99 a bug? see auto_select_hero()  */
        if (fighter[whom].ctmem == 0)
        {
            tgt = auto_select_enemy(whom, NO_STS_CHECK);
        }
        else
        {
            tgt = auto_select_hero(whom, NO_STS_CHECK);
        }
    }
    if (tgt == -1)
    {
        return 0;
    }
    fighter[whom].fighterAttackSpriteFrame = 6;
    kqCombat.fighterImageDatafileX = -1;
    kqCombat.fighterImageDatafileY = -1;
    kqCombat.battle_render(0, 0, 0);
    kqDraw.blit2screen(0, 0);
    kq_wait(150);
    kqCombat.fight(whom, tgt, 0);
    return 1;
}

void KHero::hero_choose_action(size_t fighter_index)
{
    int stop = 0, amy;
    size_t equipment_index;
    size_t ca_index;
    uint32_t sptr = 1, ptr = 0, my = 0, tt, chi[9];

    // This is going to blow up if we translate _(...) text into a language
    // where the text is longer than 8 characters.
    char ca[9][8];

    strcpy(sk_names[0], _("Rage"));
    strcpy(sk_names[1], _("Sweep"));
    strcpy(sk_names[2], _("Infuse"));
    strcpy(sk_names[3], _("Prayer"));
    strcpy(sk_names[4], _("Boost"));
    strcpy(sk_names[5], _("Cover"));
    strcpy(sk_names[6], _("Steal"));
    strcpy(sk_names[7], _("Sense"));

    if (!kqCombat.bIsEtherEffectActive[fighter_index])
    {
        return;
    }
    Game.unpress();
    fighter[fighter_index].fighterWillDefend = 0;
    fighter[fighter_index].fighterSpriteFacing = 0;
    if (pidx[fighter_index] != CORIN && pidx[fighter_index] != CASANDRA)
    {
        fighter[fighter_index].aux = 0;
    }
    while (!stop)
    {
        Game.do_check_animation();
        kqCombat.battle_render(fighter_index + 1, fighter_index + 1, 0);
        my = 0;
        strcpy(ca[my], _("Attack"));
        chi[my] = C_ATTACK;
        my++;
        if (hero_skillcheck(fighter_index))
        {
            strcpy(ca[my], sk_names[pidx[fighter_index]]);
            chi[my] = C_SKILL;
            my++;
        }
        if (fighter[fighter_index].fighterSpellEffectStats[S_MUTE] == 0 && heroc.available_spells(fighter_index) > 0)
        {
            strcpy(ca[my], _("Spell"));
            chi[my] = C_SPELL;
            my++;
        }
        if (heroc.canItemBeUsedDuringBattle())
        {
            strcpy(ca[my], _("Item"));
            chi[my] = C_ITEM;
            my++;
        }
        tt = 0;
        for (equipment_index = 0; equipment_index < NUM_EQUIPMENT; equipment_index++)
        {
            if (can_invoke_item(party[pidx[fighter_index]].eqp[equipment_index]))
            {
                tt++;
            }
        }
        if (tt > 0)
        {
            strcpy(ca[my], _("Invoke"));
            chi[my] = C_INVOKE;
            my++;
        }
        if (my > 5)
        {
            amy = 224 - (my * 8);
        }
        else
        {
            amy = 184;
        }
        kqDraw.menubox(double_buffer, 120, amy, 8, my, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        for (ca_index = 0; ca_index < my; ca_index++)
        {
            kqDraw.print_font(double_buffer, 136, ca_index * 8 + amy + 8, ca[ca_index], eFontColor::FONTCOLOR_NORMAL);
        }
        if (sptr == 1)
        {
            draw_sprite(double_buffer, menuptr, 120, ptr * 8 + amy + 8);
        }
        if (sptr == 0)
        {
            kqDraw.menubox(double_buffer, 64, amy, 6, 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
            kqDraw.print_font(double_buffer, 72, amy + 8, _("Defend"), eFontColor::FONTCOLOR_NORMAL);
        }
        if (sptr == 2)
        {
            kqDraw.menubox(double_buffer, 192, amy, 3, 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
            kqDraw.print_font(double_buffer, 200, amy + 8, _("Run"), eFontColor::FONTCOLOR_NORMAL);
        }
        kqDraw.blit2screen(0, 0);

        PlayerInput.readcontrols();
        if (PlayerInput.up)
        {
            Game.unpress();
            if (ptr > 0)
            {
                ptr--;
            }
            else
            {
                ptr = my - 1;
            }
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.down)
        {
            Game.unpress();
            if (ptr < my - 1)
            {
                ptr++;
            }
            else
            {
                ptr = 0;
            }
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.left)
        {
            Game.unpress();
            if (sptr > 0)
            {
                sptr--;
            }
        }
        if (PlayerInput.right)
        {
            Game.unpress();
            sptr++;
#ifdef DEBUGMODE
            // If we're debugging, we will force the ability to RUN
            if (debugging >= 3)
            {
                sptr = 2;
            }
            else   // This "else" isn't aligned with the following line,
                // but the following line needs to be accessed regardless
                // of whether DEBUGMODE is declared or not.
                // It also needs to run in case "debugging" is NOT >= 3.
#endif
                if (sptr - 1 > can_run)
                {
                    sptr = 1 + can_run;
                }
        }
        if (PlayerInput.balt)
        {
            Game.unpress();
            if (sptr == 0)
            {
                fighter[fighter_index].fighterWillDefend = 1;
                kqCombat.bIsEtherEffectActive[fighter_index] = false;
                stop = 1;
            }
            if (sptr == 2)
            {
                hero_run();
                kqCombat.bIsEtherEffectActive[fighter_index] = false;
                stop = 1;
            }
            if (sptr == 1)
            {
                switch (chi[ptr])
                {
                case C_ATTACK:
                    if (hero_attack(fighter_index) == 1)
                    {
                        kqCombat.bIsEtherEffectActive[fighter_index] = false;
                        stop = 1;
                    }
                    break;
                case C_ITEM:
                    if (combat_item_menu(fighter_index) == 1)
                    {
                        kqCombat.bIsEtherEffectActive[fighter_index] = false;
                        stop = 1;
                    }
                    break;
                case C_INVOKE:
                    if (hero_invoke(fighter_index) == 1)
                    {
                        kqCombat.bIsEtherEffectActive[fighter_index] = false;
                        stop = 1;
                    }
                    break;
                case C_SPELL:
                    if (heroc.combat_spell_menu(fighter_index) == 1)
                    {
                        kqCombat.bIsEtherEffectActive[fighter_index] = false;
                        stop = 1;
                    }
                    break;
                case C_SKILL:
                    if (skill_use(fighter_index) == 1)
                    {
                        kqCombat.bIsEtherEffectActive[fighter_index] = false;
                        stop = 1;
                    }
                    break;
                }
            }
        }
    }
}

void KHero::hero_init()
{
    update_equipstats();
    Raster* eb = get_cached_image("usbat.png");

    // Load all 8 fighters' stances into the `cframes` array.
    // cframes[fighter's index][]
    // cframes[][fighter's stance]
    for (unsigned int fighter_index = 0; fighter_index < numchrs; fighter_index++)
    {
        for (unsigned int frame_index = 0; frame_index < MAXCFRAMES; frame_index++)
        {
            clear_bitmap(cframes[fighter_index][frame_index]);
        }
        unsigned int current_fighter_index = pidx[fighter_index];

        unsigned int fighter_x = current_fighter_index * 64 + 192;
        unsigned int fighter_y = current_fighter_index * 32;

        // Facing away from screen (see only the fighter's back)
        blit(eb, cframes[fighter_index][0], 0, fighter_y, 0, 0, 32, 32);
        // Facing toward the screen (see only the fighter's front)
        blit(eb, cframes[fighter_index][1], 32, fighter_y, 0, 0, 32, 32);
        // Arms out (casting a spell)
        blit(eb, cframes[fighter_index][2], 64, fighter_y, 0, 0, 32, 32);
        // Dead
        blit(eb, cframes[fighter_index][3], 96, fighter_y, 0, 0, 32, 32);
        // Victory: Facing toward the screen (cheering at end of a battle)
        blit(eb, cframes[fighter_index][4], 128, fighter_y, 0, 0, 32, 32);
        // Blocking: Facing away from the screen (pushed back from enemy attack)
        blit(eb, cframes[fighter_index][5], 160, fighter_y, 0, 0, 32, 32);

        fighter_y = fighter[fighter_index].current_weapon_type * 32;

        // Attack stances, column 6 (0-based): weapon held up to strike
        blit(eb, cframes[fighter_index][6], fighter_x, fighter_y, 0, 0, 32, 32);

        // Attack stances, column 7 (0-based): weapon forward, striking
        blit(eb, cframes[fighter_index][7], fighter_x + 32, fighter_y, 0, 0, 32, 32);

        unsigned int fighter_weapon_index = party[current_fighter_index].eqp[0];

        // If `kol` is non-zero, loop through all pixels in both of the Attack
        // stances bitmaps
        // and find the light-green color in the `pal` color palette.
        // - Value "168" corresponds to entry value {27, 54, 27, 0}
        // - Value "175" corresponds to entry value {53, 63, 53, 0}
        // Swap out those "green" colors and replace them with the `kol` colors that
        // match the
        // colors that the weapons should actually be instead.
        if (fighter[fighter_index].current_weapon_type != W_NO_WEAPON && items[fighter_weapon_index].kol > 0)
        {
            for (unsigned int current_line = 0; current_line < cframes[fighter_index][0]->height; current_line++)
            {
                for (unsigned int current_pixel = 0; current_pixel < cframes[fighter_index][0]->width; current_pixel++)
                {
                    if (cframes[fighter_index][6]->getpixel(current_pixel, current_line) == 168)
                    {
                        cframes[fighter_index][6]->setpixel(current_pixel, current_line, items[fighter_weapon_index].kol);
                    }
                    else
                    {
                        if (cframes[fighter_index][6]->getpixel(current_pixel, current_line) == 175)
                        {
                            cframes[fighter_index][6]->setpixel(current_pixel, current_line, items[fighter_weapon_index].kol + 4);
                        }
                    }
                    if (cframes[fighter_index][7]->getpixel(current_pixel, current_line) == 168)
                    {
                        cframes[fighter_index][7]->setpixel(current_pixel, current_line, items[fighter_weapon_index].kol);
                    }
                    else
                    {
                        if (cframes[fighter_index][7]->getpixel(current_pixel, current_line) == 175)
                        {
                            cframes[fighter_index][7]->setpixel(current_pixel, current_line, items[fighter_weapon_index].kol + 4);
                        }
                    }
                }
            }
        }
        for (unsigned int frame_index = 0; frame_index < MAXCFRAMES; frame_index++)
        {
            tcframes[fighter_index][frame_index] = kqDraw.copy_bitmap(tcframes[fighter_index][frame_index], cframes[fighter_index][frame_index]);
        }

        fighter[fighter_index].fighterImageDatafileWidth = 32;
        fighter[fighter_index].fighterImageDatafileHeight = 32;
        fighter[fighter_index].fighterAttackSpriteFrame = 0;
    }
}

int KHero::hero_invoke(int whom)
{
    int stop = 0, ptr = 0;
    int dud;

    fullblit(double_buffer, back);
    dud = pidx[whom];
    while (!stop)
    {
        Game.do_check_animation();
        fullblit(back, double_buffer);
        draw_invokable(dud);
        draw_sprite(double_buffer, menuptr, 72, ptr * 8 + 88);
        kqDraw.blit2screen(0, 0);

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
        if (PlayerInput.balt)
        {
            Game.unpress();
            if (can_invoke_item(party[dud].eqp[ptr]))
            {
                if (hero_invokeitem(whom, party[dud].eqp[ptr]) == 1)
                {
                    stop = 2;
                }
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
    return stop - 1;
}

int KHero::hero_invokeitem(size_t attacker_fighter_index, size_t item_index)
{
    ePIDX defender_fighter_index;
    eTarget tgt = (eTarget)items[item_index].tgt;
    if (tgt <= TGT_ALLY_ALL && tgt >= TGT_ALLY_ONE)
    {
        defender_fighter_index = select_hero(attacker_fighter_index, tgt, 0);
    }
    else if (tgt >= TGT_ENEMY_ONE && tgt <= TGT_ENEMY_ALL)
    {
        defender_fighter_index = select_enemy(attacker_fighter_index, tgt);
    }
    else
    {
        defender_fighter_index = PIDX_UNDEFINED;
    }

    if (defender_fighter_index == PIDX_UNDEFINED)
    {
        return 0;
    }

    if (items[item_index].imb > 0)
    {
        cast_imbued_spell(attacker_fighter_index, items[item_index].imb, items[item_index].stats[A_ATT], defender_fighter_index);
        return 1;
    }

    /* This will likely become a separate function, but here is
     * where we are invoking items.
     */
    if (item_index == I_STAFF1)
    {
        strcpy(attack_string, _("Neutralize Poison"));
        draw_spellsprite(0, 1, 27, 0);
        for (unsigned int fighter_index = 0; fighter_index < numchrs; fighter_index++)
        {
            if (fighter[fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD)
            {
                fighter[fighter_index].fighterSpellEffectStats[S_POISON] = 0;
            }
        }
    }
    if (item_index == I_ROD1)
    {
        unsigned int random_fighter_index = kqrandom->random_range_exclusive(1, 4);
        strcpy(attack_string, _("Magic Missiles"));
        display_attack_string = true;
        kqCombat.ta[defender_fighter_index] = 0;
        for (unsigned fighter_index = 0; fighter_index < random_fighter_index; fighter_index++)
        {
            if (fighter[defender_fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD)
            {
                draw_attacksprite(defender_fighter_index, 0, 4, 1);
                special_damage_oneall_enemies(attacker_fighter_index, 16, -1, defender_fighter_index, 0);
            }
        }
        display_attack_string = false;
    }
    return 1;
}

void KHero::hero_run()
{
    int a, b = 0, c = 0, bt = 0, ct = 0, fr, fx, fy, goldDropped = 0;
    size_t fighter_index;

    if (kqCombat.getChanceEnemiesSurpriseYou() == 1)
    {
        a = 125;
    }
    else
    {
        a = 74;
    }
    for (fighter_index = 0; fighter_index < numchrs; fighter_index++)
    {
        if (fighter[fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD)
        {
            b++;
            bt += fighter[fighter_index].fighterStats[A_SPD];
        }
    }

    if (b == 0)
    {
        Game.program_death(_("Fatal error: How can a dead party run?"));
    }
    else
    {
        bt = bt / b;
    }

    for (fighter_index = MAX_PARTY_SIZE; fighter_index < MAX_PARTY_SIZE + kqCombat.num_enemies; fighter_index++)
    {
        if (fighter[fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD)
        {
            c++;
            ct += fighter[fighter_index].fighterStats[A_SPD];
        }
    }
    if (c == 0)
    {
        Game.program_death(_("Fatal error: Why are the heroes running from dead enemies?"));
    }
    else
    {
        ct = ct / c;
    }

    if (bt > ct)
    {
        a += 25;
    }
    else if (bt < ct)
    {
        a -= 25;
    }

    const auto random1 = kqrandom->random_range_exclusive(0, 100);
    if (random1 < a)
    {
        const auto random2 = kqrandom->random_range_exclusive(0, 100);
        if (random2 < (100 - a))
        {
            goldDropped = b * fighter[MAX_PARTY_SIZE].fighterLevel * c;
            if (gp < goldDropped)
            {
                goldDropped = gp;
            }
            gp -= goldDropped;
        }
    }
    else
    {
        kqDraw.menubox(double_buffer, 84, 32, 17, 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        kqDraw.print_font(double_buffer, 92, 40, _("Could not escape!"), eFontColor::FONTCOLOR_NORMAL);
        kqDraw.blit2screen(0, 0);
        Game.wait_enter();
        return;
    }

    if (goldDropped > 0)
    {
        sprintf(strbuf, _("Escaped! Dropped %d gold!"), goldDropped);
    }
    else
    {
        sprintf(strbuf, _("Escaped!"));
    }

    size_t stringLength = strlen(strbuf) * 4;

    /* TT: slow_computer addition for speed-ups */
    const unsigned int count = slow_computer ? 3 : 20;

    for (unsigned int i = 0; i < 5; i++)
    {
        for (unsigned int frame_counter = 0; frame_counter < count; frame_counter++)
        {
            clear_bitmap(double_buffer);
            kqDraw.menubox(double_buffer, 152 - stringLength, 32, strlen(strbuf), 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
            kqDraw.print_font(double_buffer, 160 - stringLength, 40, strbuf, eFontColor::FONTCOLOR_NORMAL);
            for (fighter_index = 0; fighter_index < numchrs; fighter_index++)
            {
                fx = fighter[fighter_index].fighterImageDatafileX;
                fy = fighter[fighter_index].fighterImageDatafileY;
                fr = 0;
                if (frame_counter > count / 2) // integer division for count/2
                {
                    fr++;
                }

                if (fighter[fighter_index].fighterSpellEffectStats[S_DEAD] == eDeathType::NOT_DEAD)
                {
                    draw_sprite(double_buffer, frames[pidx[fighter_index]][fr], fx, fy);
                }
            }
            kqDraw.blit2screen(0, 0);
            kq_wait(10);
        }
    }
    revert_equipstats();
    kqCombat.combatend = ESCAPED_COMBAT;
}
