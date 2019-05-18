/*! \file
 * \brief Effects and animations
 *
 * \author JB
 * \date ???????
 */

#include <memory>
#include <stdio.h>
#include <string.h>

#include "combat.h"
#include "draw.h"
#include "effects.h"
#include "gfx.h"
#include "imgcache.h"
#include "itemmenu.h"
#include "kq.h"
#include "magic.h"
#include "music.h"
#include "platform.h"
#include "res.h"
#include "setup.h"
#include "ssprites.h"
#include "timing.h"

void death_animation(size_t target_fighter_index, int target_all_flag)
{
    int dx, dy, p;
    int color_range;
    size_t fighter_index;
    size_t start_fighter_index, num_targets;

    // TT: used for the slow_computer routine
    int count;

    if (target_fighter_index < MAX_PARTY_SIZE)
    {
        return;
    }
    if (target_all_flag == 1)
    {
        start_fighter_index = MAX_PARTY_SIZE;
        num_targets = kqCombat.num_enemies;
    }
    else
    {
        start_fighter_index = target_fighter_index;
        num_targets = 1;
    }
    kqCombat.fighterImageDatafileX = -1;
    kqCombat.fighterImageDatafileY = -1;
    play_effect(24, 128);
    kqCombat.battle_render(0, 0, 0);
    fullblit(double_buffer, back);

    // TT: slow_computer addition for speed-ups
    count = (slow_computer ? 4 : 1);
    for (p = 0; p < 2; p++)
    {
        // TT: slow_computer additions for speed-ups
        for (color_range = 0; color_range < 16; color_range += count)
        {
            kqDraw.convert_cframes(target_fighter_index, 1, 15 - (color_range / 2), target_all_flag);
            for (fighter_index = start_fighter_index; fighter_index < start_fighter_index + num_targets; fighter_index++)
            {
                if (kqCombat.deffect[fighter_index] == 1)
                {
                    dx = fighter[fighter_index].fighterImageDatafileX + (fighter[fighter_index].fighterImageDatafileWidth / 2);
                    dy = fighter[fighter_index].fighterImageDatafileY + (fighter[fighter_index].fighterImageDatafileHeight / 2);
                    if (p == 0)
                    {
                        circlefill(double_buffer, dx, dy, color_range, 0);
                        kqCombat.draw_fighter(fighter_index, 0);
                    }
                    else
                    {
                        circlefill(double_buffer, dx, dy, 15 - color_range, 0);
                    }
                }
            }
            kqDraw.blit2screen(0, 0);
            kq_wait(30);
            fullblit(back, double_buffer);
        }
    }
    for (fighter_index = start_fighter_index; fighter_index < start_fighter_index + num_targets; fighter_index++)
    {
        if (kqCombat.deffect[fighter_index] == 1)
        {
            kqCombat.deffect[fighter_index] = 0;
        }
    }
    kqDraw.revert_cframes(target_fighter_index, target_all_flag);
    kqCombat.battle_render(0, 0, 0);
    kqDraw.blit2screen(0, 0);
}

void display_amount(size_t target_fighter_index, eFont font_color, int multiple_target)
{
    int dx, dy, sprite_height;
    uint32_t c, p;
    size_t fighter_index, start_fighter_index, num_fighters;
    size_t string_length;

    /* TT: used for the slow_computer routine */
    uint32_t pcount, ccount;

    if (multiple_target == 1)
    {
        if (target_fighter_index < MAX_PARTY_SIZE)
        {
            start_fighter_index = 0;
            num_fighters = numchrs;
        }
        else
        {
            start_fighter_index = MAX_PARTY_SIZE;
            num_fighters = kqCombat.num_enemies;
        }
    }
    else
    {
        start_fighter_index = target_fighter_index;
        num_fighters = 1;
    }
    kqCombat.fighterImageDatafileX = -1;
    kqCombat.fighterImageDatafileY = -1;
    kqCombat.battle_render(0, 0, 0);
    fullblit(double_buffer, back);

    // TT: slow_computer addition for speed-ups
    pcount = (slow_computer ? 1 : 2);
    ccount = (slow_computer ? 3 : 12);

    // TT: slow_computer addition for speed-ups
    for (p = 0; p < pcount; p++)
    {
        for (c = 0; c < ccount; c++)
        {
            fullblit(back, double_buffer);
            for (fighter_index = start_fighter_index; fighter_index < start_fighter_index + num_fighters; fighter_index++)
            {
                if (is_active(fighter_index) == 1)
                {
                    dx = fighter[fighter_index].fighterImageDatafileX + (fighter[fighter_index].fighterImageDatafileWidth / 2);
                    if (fighter[fighter_index].fighterImageDatafileHeight <= 64)
                    {
                        dy = fighter[fighter_index].fighterImageDatafileY;
                    }
                    else
                    {
                        dy = fighter[fighter_index].fighterImageDatafileY + fighter[fighter_index].fighterImageDatafileHeight - 8;
                    }
                    if (kqCombat.ta[fighter_index] == NODISPLAY)
                    {
                        sprintf(strbuf, "_");
                    }
                    else
                    {
                        sprintf(strbuf, "%d", abs(kqCombat.ta[fighter_index]));
                    }
                    string_length = strlen(strbuf) * 3;
                    eFont new_font_color = font_color;
                    if (font_color == eFont::FONT_DECIDE)
                    {
                        new_font_color = (kqCombat.ta[fighter_index] > 0 ? eFont::FONT_YELLOW : eFont::FONT_WHITE);
                    }
                    kqCombat.draw_fighter(fighter_index, 0);

                    if (p == 0)
                    {
                        sprite_height = dy - c;
                    }
                    else
                    {
                        sprite_height = dy - 9 + c;
                    }

                    if (kqCombat.ta[fighter_index] == MISS)
                    {
                        draw_sprite(double_buffer, missbmp, dx - 10, sprite_height);
                    }
                    else
                    {
                        kqDraw.print_num(double_buffer, dx - string_length, sprite_height, strbuf, new_font_color);
                    }
                }
            }
            kqDraw.blit2screen(0, 0);
            kq_wait(30);
        }
    }
    kqCombat.battle_render(0, 0, 0);
    kqDraw.blit2screen(0, 0);
}

void draw_attacksprite(size_t target_fighter_index, int multiple_target, size_t magic_effect_index, int shows)
{
    int a, dx, dy;
    size_t fighter_index;
    size_t num_fighters, start_fighter_index;
    Raster* eb = get_cached_image(eff[magic_effect_index].ename);

    if (multiple_target == 1)
    {
        if (target_fighter_index < MAX_PARTY_SIZE)
        {
            start_fighter_index = 0;
            num_fighters = numchrs;
        }
        else
        {
            start_fighter_index = MAX_PARTY_SIZE;
            num_fighters = kqCombat.num_enemies;
        }
    }
    else
    {
        start_fighter_index = target_fighter_index;
        num_fighters = 1;
    }

    if (target_fighter_index < MAX_PARTY_SIZE)
    {
        for (fighter_index = start_fighter_index; fighter_index < start_fighter_index + num_fighters; fighter_index++)
        {
            fighter[fighter_index].fighterAttackSpriteFrame = 5;
        }
    }
    kqCombat.fighterImageDatafileX = -1;
    kqCombat.fighterImageDatafileY = -1;
    kqCombat.battle_render(0, 0, 0);
    fullblit(double_buffer, back);
    if (multiple_target == 0)
    {
        if (kqCombat.ta[start_fighter_index] == MISS)
        {
            play_effect(SND_MENU, 128);
        }
        else
        {
            play_effect(eff[magic_effect_index].snd, 128);
        }
    }
    else
    {
        play_effect(eff[magic_effect_index].snd, 128);
    }
    for (a = 0; a < eff[magic_effect_index].numf; a++)
    {
        for (fighter_index = start_fighter_index; fighter_index < start_fighter_index + num_fighters; fighter_index++)
        {
            if (is_active(fighter_index) == 1)
            {
                dx = fighter[fighter_index].fighterImageDatafileX + (fighter[fighter_index].fighterImageDatafileWidth / 2) - (eff[magic_effect_index].xsize / 2);
                dy = fighter[fighter_index].fighterImageDatafileY + (fighter[fighter_index].fighterImageDatafileHeight / 2) - (eff[magic_effect_index].ysize / 2);
                kqCombat.draw_fighter(fighter_index, 0);
                if (shows == 1 && fighter[fighter_index].fighterSpellEffectStats[S_SHIELD] > 0)
                {
                    // The shield sprite in MISC is 48x48 pixels, so center it over the
                    // fighter.
                    draw_trans_sprite(double_buffer, b_shield,
                        fighter[fighter_index].fighterImageDatafileX + (fighter[fighter_index].fighterImageDatafileWidth / 2) - 24,
                        fighter[fighter_index].fighterImageDatafileY + (fighter[fighter_index].fighterImageDatafileHeight / 2) - 24);
                }
                masked_blit(eb, double_buffer,
                    0, eff[magic_effect_index].ysize * a,
                    dx, dy,
                    eff[magic_effect_index].xsize, eff[magic_effect_index].ysize);
            }
        }
        kqDraw.blit2screen(0, 0);
        kq_wait(eff[magic_effect_index].delay);
        fullblit(back, double_buffer);
    }
    if (target_fighter_index < MAX_PARTY_SIZE)
    {
        for (fighter_index = start_fighter_index; fighter_index < start_fighter_index + num_fighters; fighter_index++)
        {
            fighter[fighter_index].fighterAttackSpriteFrame = 0;
        }
    }
}

void draw_castersprite(size_t caster_fighter_index, int new_pal_color)
{
    int dx, dy;
    unsigned int frame_index;
    unsigned int pixel_row, pixel_col;
    Raster* cs = get_cached_image("caster2.png");

    // Re-colorize the two-tone image by replacing its value in the palette
    // with another palette color entry.
    for (pixel_row = 0; pixel_row < (unsigned int)cs->height; pixel_row++)
    {
        for (pixel_col = 0; pixel_col < (unsigned int)cs->width; pixel_col++)
        {
            // Pixel color 5 in PALETTE 'pal' equals {20, 20, 20, 0}
            if (cs->getpixel(pixel_col, pixel_row) == 5)
            {
                cs->setpixel(pixel_col, pixel_row, new_pal_color);
            }
            else if (cs->getpixel(pixel_col, pixel_row) == 7)
            {
                cs->setpixel(pixel_col, pixel_row, new_pal_color + 2);
            }
        }
    }
    kqCombat.fighterImageDatafileX = -1;
    kqCombat.fighterImageDatafileY = -1;
    fighter[caster_fighter_index].fighterAttackSpriteFrame = 2;
    display_attack_string = true;
    kqCombat.battle_render(0, 0, 0);
    display_attack_string = false;
    fullblit(double_buffer, back);
    play_effect(22, 128);

    // This animation has 10 frames, each 32 pixels tall/wide.
    for (frame_index = 0; frame_index < 10; frame_index++)
    {
        if (is_active(caster_fighter_index) == 1)
        {
            dx = fighter[caster_fighter_index].fighterImageDatafileX + (fighter[caster_fighter_index].fighterImageDatafileWidth / 2);
            dy = fighter[caster_fighter_index].fighterImageDatafileY + (fighter[caster_fighter_index].fighterImageDatafileHeight / 2);
            kqCombat.draw_fighter(caster_fighter_index, 0);
            masked_blit(cs, double_buffer, 0, frame_index * 32, dx - 16, dy - 16, 32, 32);
        }
        kqDraw.blit2screen(0, 0);
        kq_wait(120);
        fullblit(back, double_buffer);
    }
    fighter[caster_fighter_index].fighterAttackSpriteFrame = 0;
}

void draw_hugesprite(size_t target_fighter_index, int hx, int hy, size_t effect_index, int shows)
{
    size_t frame_index;
    size_t fighter_index;
    size_t start_fighter_index, num_fighters;
    Raster* eb = get_cached_image(eff[effect_index].ename);

    kqDraw.convert_cframes(target_fighter_index, eff[effect_index].kolor - 3, eff[effect_index].kolor + 3, 1);
    if (target_fighter_index < MAX_PARTY_SIZE)
    {
        start_fighter_index = 0;
        num_fighters = numchrs;
    }
    else
    {
        start_fighter_index = MAX_PARTY_SIZE;
        num_fighters = kqCombat.num_enemies;
    }
    kqCombat.fighterImageDatafileX = -1;
    kqCombat.fighterImageDatafileY = -1;
    display_attack_string = true;
    kqCombat.battle_render(0, 0, 0);
    display_attack_string = false;
    fullblit(double_buffer, back);
    play_effect(eff[effect_index].snd, 128);
    for (frame_index = 0; frame_index < eff[effect_index].numf; frame_index++)
    {
        if (eff[effect_index].orient == 0)
        {
            masked_blit(eb, double_buffer, 0, eff[effect_index].ysize * frame_index, hx, hy, eff[effect_index].xsize, eff[effect_index].ysize);
        }
        for (fighter_index = start_fighter_index; fighter_index < start_fighter_index + num_fighters; fighter_index++)
        {
            if (is_active(fighter_index) == 1)
            {
                if (shows == 1 && fighter[fighter_index].fighterSpellEffectStats[S_RESIST] > 0)
                {
                    draw_trans_sprite(double_buffer, b_shell,
                        fighter[fighter_index].fighterImageDatafileX + (fighter[fighter_index].fighterImageDatafileWidth / 2) - 24,
                        fighter[fighter_index].fighterImageDatafileY + (fighter[fighter_index].fighterImageDatafileHeight / 2) - 24);
                }
                kqCombat.draw_fighter(fighter_index, 0);
            }
        }
        if (eff[effect_index].orient == 1)
        {
            masked_blit(eb, double_buffer, 0, eff[effect_index].ysize * frame_index, hx, hy, eff[effect_index].xsize, eff[effect_index].ysize);
        }
        kqDraw.blit2screen(0, 0);
        kq_wait(eff[effect_index].delay);
        fullblit(back, double_buffer);
    }
    kqDraw.revert_cframes(target_fighter_index, 1);
}

void draw_spellsprite(size_t target_fighter_index, int multiple_target, size_t effect_index, int shows)
{
    int dx, dy = 0;
    size_t num_frames;
    size_t start_fighter_index, num_fighers, fighter_index;
    Raster* eb = get_cached_image(eff[effect_index].ename);

    kqDraw.convert_cframes(target_fighter_index, eff[effect_index].kolor - 3, eff[effect_index].kolor + 3, multiple_target);
    if (multiple_target == 1)
    {
        if (target_fighter_index < MAX_PARTY_SIZE)
        {
            start_fighter_index = 0;
            num_fighers = numchrs;
        }
        else
        {
            start_fighter_index = MAX_PARTY_SIZE;
            num_fighers = kqCombat.num_enemies;
        }
    }
    else
    {
        start_fighter_index = target_fighter_index;
        num_fighers = 1;
    }
    kqCombat.fighterImageDatafileX = -1;
    kqCombat.fighterImageDatafileY = -1;
    display_attack_string = true;
    kqCombat.battle_render(0, 0, 0);
    display_attack_string = false;
    fullblit(double_buffer, back);
    play_effect(eff[effect_index].snd, 128);
    for (num_frames = 0; num_frames < eff[effect_index].numf; num_frames++)
    {
        for (fighter_index = start_fighter_index; fighter_index < start_fighter_index + num_fighers; fighter_index++)
        {
            if (is_active(fighter_index) == 1)
            {
                dx = fighter[fighter_index].fighterImageDatafileX + (fighter[fighter_index].fighterImageDatafileWidth / 2) - (eff[effect_index].xsize / 2);
                switch (eff[effect_index].orient)
                {
                case 0:
                    dy = fighter[fighter_index].fighterImageDatafileY + fighter[fighter_index].fighterImageDatafileHeight - eff[effect_index].ysize;
                    break;
                case 1:
                    dy = fighter[fighter_index].fighterImageDatafileY + (fighter[fighter_index].fighterImageDatafileHeight / 2) - (eff[effect_index].ysize / 2);
                    break;
                case 2:
                    dy = fighter[fighter_index].fighterImageDatafileY + eff[effect_index].ysize;
                    break;
                }
                kqCombat.draw_fighter(fighter_index, 0);
                if (shows == 1 && fighter[fighter_index].fighterSpellEffectStats[S_RESIST] > 0)
                {
                    draw_trans_sprite(double_buffer, b_shell,
                        fighter[fighter_index].fighterImageDatafileX + (fighter[fighter_index].fighterImageDatafileWidth / 2) - 24,
                        fighter[fighter_index].fighterImageDatafileY + (fighter[fighter_index].fighterImageDatafileHeight / 2) - 24);
                }
                masked_blit(eb, double_buffer, 0, eff[effect_index].ysize * num_frames, dx, dy, eff[effect_index].xsize, eff[effect_index].ysize);
            }
        }
        kqDraw.blit2screen(0, 0);
        kq_wait(eff[effect_index].delay);
        fullblit(back, double_buffer);
    }
    kqDraw.revert_cframes(target_fighter_index, multiple_target);
}

void fight_animation(size_t target_fighter_index, size_t fighter_index, int multiple_target)
{
    size_t magic_effect_index;
    size_t fighter_weapon_index;

    if (fighter_index < MAX_PARTY_SIZE)
    {
        fighter_weapon_index = party[pidx[fighter_index]].eqp[EQP_WEAPON];
        magic_effect_index = items[fighter_weapon_index].eff;
    }
    else
    {
        magic_effect_index = fighter[fighter_index].current_weapon_type;
    }
    draw_attacksprite(target_fighter_index, multiple_target, magic_effect_index, 1);
}

int is_active(int guy)
{
    return (fighter[guy].fighterSpellEffectStats[S_DEAD] == deadeffect ? eDeathType::IS_DEAD : eDeathType::NOT_DEAD);
}
