/*! \file
 * \brief Save and Load game
 * \author JB
 * \date ????????
 *
 * Support for saving and loading games.
 * Also includes the main menu and the system menu
 * (usually accessible by pressing ESC).
 *
 * \todo PH Do we _really_ want things like controls and screen mode to be saved/loaded ?
 */

#include <ctype.h>
#include <stdio.h>
#include <string>

#include "combat.h"
#include "constants.h"
#include "credits.h"
#include "disk.h"
#include "draw.h"
#include "gfx.h"
#include "imgcache.h"
#include "input.h"
#include "intrface.h"
#include "kq.h"
#include "magic.h"
#include "masmenu.h"
#include "menu.h"
#include "music.h"
#include "platform.h"
#include "res.h"
#include "setup.h"
#include "sgame.h"
#include "shopmenu.h"
#include "structs.h"
#include "timing.h"

const size_t KSaveGame::NUMSG = 20;

KSaveGame::KSaveGame()
    : save_ptr{ 0 }
    , top_pointer{ 0 }
    , max_onscreen{ 5 }
{
    savegame.resize(NUMSG);
}

KSaveGame::~KSaveGame()
{
}

int KSaveGame::confirm_action()
{
    int stop = 0;
    int pointer_offset = (save_ptr - top_pointer) * 48;
    // Nothing in this slot so confirm without asking.
    if (savegame[save_ptr].num_characters == 0)
    {
        return 1;
    }
    fullblit(double_buffer, back);
    kqDraw.menubox(double_buffer, 128, pointer_offset + 12, 14, 1, eMenuBoxColor::DARKBLUE);
    kqDraw.print_font(double_buffer, 136, pointer_offset + 20, _("Confirm/Cancel"), eFontColor::FONTCOLOR_NORMAL);
    kqDraw.blit2screen(0, 0);
    fullblit(back, double_buffer);
    while (!stop)
    {
        PlayerInput.readcontrols();
        if (PlayerInput.balt)
        {
            Game.unpress();
            return 1;
        }
        if (PlayerInput.bctrl)
        {
            Game.unpress();
            return 0;
        }
        Game.kq_yield();
    }
    return 0;
}

/*! \brief Confirm that the player really wants to quit
 *
 * Ask the player if she/he wants to quit, yes or no.
 * \date 20050119
 * \author PH
 *
 * \returns 1=quit 0=don't quit
 */
static int confirm_quit()
{
    const char* opts[2];
    int ans;

    opts[0] = _("Yes");
    opts[1] = _("No");
    /*strcpy(opts[1], "No"); */
    ans = kqDraw.prompt_ex(0, _("Are you sure you want to quit this game?"), opts, 2);
    return ans == 0 ? 1 : 0;
}

void KSaveGame::delete_game()
{
    bool stop = false;
    int pointer_offset = (save_ptr - top_pointer) * 48;

    sprintf(strbuf, "sg%d.sav", save_ptr);
    int remove_result = remove(kqres(SAVE_DIR, strbuf).c_str());
    if (remove_result == 0)
    {
        kqDraw.menubox(double_buffer, 128, pointer_offset + 12, 12, 1, eMenuBoxColor::DARKBLUE);
        kqDraw.print_font(double_buffer, 136, pointer_offset + 20, _("File Deleted"), eFontColor::FONTCOLOR_NORMAL);

        s_sgstats& stats = savegame[save_ptr];
        stats.num_characters = 0;
        stats.gold = 0;
        stats.time = 0;
    }
    else
    {
        kqDraw.menubox(double_buffer, 128, pointer_offset + 12, 16, 1, eMenuBoxColor::DARKBLUE);
        kqDraw.print_font(double_buffer, 136, pointer_offset + 20, _("File Not Deleted"), eFontColor::FONTCOLOR_NORMAL);
    }
    kqDraw.blit2screen(0, 0);
    fullblit(back, double_buffer);

    while (!stop)
    {
        PlayerInput.readcontrols();
        if (PlayerInput.balt || PlayerInput.bctrl)
        {
            Game.unpress();
            stop = true;
        }
        Game.kq_yield();
    }
}

int KSaveGame::load_game()
{
    sprintf(strbuf, "sg%ld.xml", save_ptr);
    load_game_xml(kqres(SAVE_DIR, strbuf).c_str());
    timer_count = 0;
    ksec = 0;
    hold_fade = 0;
    Game.change_map(Game.GetCurmap(), allEntitiesOnTheMap[0].tilex, allEntitiesOnTheMap[0].tiley, allEntitiesOnTheMap[0].tilex, allEntitiesOnTheMap[0].tiley);
    /* Set music and sound volume */
    set_volume(gsvol, -1);
    Music.set_music_volume(((float)gmvol) / 250.0);
    return 1;
}

void KSaveGame::load_sgstats()
{
    for (int sg = 0; sg < NUMSG; ++sg)
    {
        char buf[32];
        sprintf(buf, "sg%u.xml", sg);
        std::string path = kqres(SAVE_DIR, std::string(buf));
        s_sgstats& stats = savegame[sg];
        if (exists(path.c_str()) && (load_stats_only(path.c_str(), stats) != 0))
        {
            // Not found (which is OK), so zero out the struct
            stats.num_characters = 0;
        }
    }
}

int KSaveGame::save_game()
{
    sprintf(strbuf, "sg%ld.xml", save_ptr);
    int rc = save_game_xml(kqres(SAVE_DIR, strbuf).c_str());
    if (rc)
    {
        savegame[save_ptr] = s_sgstats::get_current();
    }
    return rc;
}

int KSaveGame::saveload(int am_saving)
{
    int stop = 0;

    // Have no more than 5 savestate boxes onscreen, but fewer if NUMSG < 5
    max_onscreen = 5;
    if (max_onscreen > NUMSG)
    {
        max_onscreen = NUMSG;
    }

    play_effect(SND_MENU, 128);
    while (!stop)
    {
        Game.do_check_animation();
        double_buffer->fill(0);
        show_sgstats(am_saving);
        kqDraw.blit2screen(0, 0);

        PlayerInput.readcontrols();
        if (PlayerInput.up)
        {
            Game.unpress();
            if (save_ptr > 0)
            {
                save_ptr--;
            }
            else
            {
                save_ptr = NUMSG - 1;
            }

            // Determine whether to update TOP
            if (save_ptr < top_pointer)
            {
                top_pointer--;
            }
            else if (save_ptr == NUMSG - 1)
            {
                top_pointer = NUMSG - max_onscreen;
            }

            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.down)
        {
            Game.unpress();
            if (save_ptr < NUMSG - 1)
            {
                save_ptr++;
            }
            else
            {
                save_ptr = 0;
            }

            // Determine whether to update TOP
            if (save_ptr >= top_pointer + max_onscreen)
            {
                top_pointer++;
            }
            else if (save_ptr == 0)
            {
                top_pointer = 0;
            }

            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.right)
        {
            Game.unpress();
            if (am_saving < 2)
            {
                am_saving = am_saving + 2;
            }
        }
        if (PlayerInput.left)
        {
            Game.unpress();
            if (am_saving >= 2)
            {
                am_saving = am_saving - 2;
            }
        }
        if (PlayerInput.balt)
        {
            Game.unpress();
            switch (am_saving)
            {
            case 0: // Load
                if (savegame[save_ptr].num_characters != 0)
                {
                    if (load_game() == 1)
                    {
                        stop = 2;
                    }
                    else
                    {
                        stop = 1;
                    }
                }
                break;
            case 1: // Save
                if (confirm_action() == 1)
                {
                    if (save_game() == 1)
                    {
                        stop = 2;
                    }
                    else
                    {
                        stop = 1;
                    }
                }
                break;
            case 2: // Delete (was LOAD) previously
            case 3: // Delete (was SAVE) previously
                if (savegame[save_ptr].num_characters != 0)
                {
                    if (confirm_action() == 1)
                    {
                        delete_game();
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
    }
    return stop - 1;
}

void KSaveGame::show_sgstats(int saving)
{
    int a, hx, hy, b;
    int pointer_offset;

    /* TT UPDATE:
     * More than 5 save states!  Hooray!
     *
     * Details of changes:
     *
     * If we want to have, say, 10 save games instead of 5, we can only show
     * 5 on the screen at any given time.  Therefore, we will need to print the
     * menuboxes 0..4 and have an up/down arrow indicator to show there are
     * more selections to choose from.
     *
     * To draw the menuboxes, we need to keep track of the TOP visible savegame
     * (0..5), and alter the rest accordingly.
     *
     * When the 5th on-screen box is selected and DOWN is pressed, move down by
     * one menubox (shift all savegames up one) so 1..5 are showing (vs 0..4).
     * When the 5th on-screen box is SG9 (10th savegame), loop up to the top of
     * the savegames (save_ptr=0, top_pointer=0).
     */

    pointer_offset = (save_ptr - top_pointer) * 48;
    if (saving == 0)
    {
        kqDraw.menubox(double_buffer, 0, pointer_offset + 12, 7, 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        kqDraw.print_font(double_buffer, 8, pointer_offset + 20, _("Loading"), eFontColor::FONTCOLOR_GOLD);
    }
    else if (saving == 1)
    {
        kqDraw.menubox(double_buffer, 8, pointer_offset + 12, 6, 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        kqDraw.print_font(double_buffer, 16, pointer_offset + 20, _("Saving"), eFontColor::FONTCOLOR_GOLD);
    }
    else if (saving == 2 || saving == 3)
    {
        kqDraw.menubox(double_buffer, 8, pointer_offset + 12, 6, 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        kqDraw.print_font(double_buffer, 16, pointer_offset + 20, _("Delete"), eFontColor::FONTCOLOR_RED);
    }

    if (top_pointer > 0)
    {
        draw_sprite(double_buffer, upptr, 32, 0);
    }
    if (top_pointer < NUMSG - max_onscreen)
    {
        draw_sprite(double_buffer, dnptr, 32, KQ_SCREEN_H - 8);
    }

    for (size_t sg = top_pointer; sg < top_pointer + max_onscreen; sg++)
    {
        s_sgstats& stats = savegame[sg];
        pointer_offset = (sg - top_pointer) * 48;
        if (sg == save_ptr)
        {
            kqDraw.menubox(double_buffer, 72, pointer_offset, 29, 4, eMenuBoxColor::DARKBLUE);
        }
        else
        {
            kqDraw.menubox(double_buffer, 72, pointer_offset, 29, 4, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        }

        if (savegame[sg].num_characters == -1)
        {
            kqDraw.print_font(double_buffer, 136, pointer_offset + 20, _("Wrong version"), eFontColor::FONTCOLOR_NORMAL);
        }
        else
        {
            if (stats.num_characters == 0)
            {
                kqDraw.print_font(double_buffer, 168, pointer_offset + 20, _("Empty"), eFontColor::FONTCOLOR_NORMAL);
            }
            else
            {
                for (a = 0; a < stats.num_characters; a++)
                {
                    auto& chr = stats.characters[a];
                    hx = a * 72 + 84;
                    hy = pointer_offset + 12;
                    draw_sprite(double_buffer, frames[chr.id][1], hx, hy + 4);
                    sprintf(strbuf, _("L: %02d"), chr.level);
                    kqDraw.print_font(double_buffer, hx + 16, hy, strbuf, eFontColor::FONTCOLOR_NORMAL);
                    kqDraw.print_font(double_buffer, hx + 16, hy + 8, _("H:"), eFontColor::FONTCOLOR_NORMAL);
                    kqDraw.print_font(double_buffer, hx + 16, hy + 16, _("M:"), eFontColor::FONTCOLOR_NORMAL);
                    rectfill(double_buffer, hx + 33, hy + 9, hx + 65, hy + 15, 2);
                    rectfill(double_buffer, hx + 32, hy + 8, hx + 64, hy + 14, 35);
                    rectfill(double_buffer, hx + 33, hy + 17, hx + 65, hy + 23, 2);
                    rectfill(double_buffer, hx + 32, hy + 16, hx + 64, hy + 22, 19);
                    b = chr.hp * 32 / 100;
                    rectfill(double_buffer, hx + 32, hy + 9, hx + 32 + b, hy + 13, 41);
                    b = chr.mp * 32 / 100;
                    rectfill(double_buffer, hx + 32, hy + 17, hx + 32 + b, hy + 21, 25);
                }
                sprintf(strbuf, _("T %u:%02u"), stats.time / 60, stats.time % 60);
                kqDraw.print_font(double_buffer, 236, pointer_offset + 12, strbuf, eFontColor::FONTCOLOR_NORMAL);
                sprintf(strbuf, _("G %u"), stats.gold);
                kqDraw.print_font(double_buffer, 236, pointer_offset + 28, strbuf, eFontColor::FONTCOLOR_NORMAL);
            }
        }
    }
}

int KSaveGame::start_menu(int skip_splash)
{
    int stop = 0, ptr = 0, redraw = 1, a;
    unsigned int fade_color;
    Raster* staff, *dudes, *tdudes;
    Raster* title = get_cached_image("title.png");
#ifdef DEBUGMODE
    if (debugging == 0)
    {
#endif
        Music.play_music("oxford.s3m", 0);
        /* Play splash (with the staff and the heroes in circle */
        if (skip_splash == 0)
        {
            Raster* splash = get_cached_image("kqt.png");
            staff = new Raster(72, 226);
            dudes = new Raster(112, 112);
            tdudes = new Raster(112, 112);
            blit(splash, staff, 0, 7, 0, 0, 72, 226);
            blit(splash, dudes, 80, 0, 0, 0, 112, 112);
            double_buffer->fill(0);
            blit(staff, double_buffer, 0, 0, 124, 22, 72, 226);
            kqDraw.blit2screen(0, 0);

            kq_wait(1000);
            for (a = 0; a < 42; a++)
            {
                stretch_blit(staff, double_buffer, 0, 0, 72, 226, 124 - (a * 32), 22 - (a * 96), 72 + (a * 64), 226 + (a * 192));
                kqDraw.blit2screen(0, 0);
                kq_wait(100);
            }
            for (a = 0; a < 5; a++)
            {
                kqDraw.color_scale(dudes, tdudes, 53 - a, 53 + a);
                draw_sprite(double_buffer, tdudes, 106, 64);
                kqDraw.blit2screen(0, 0);
                kq_wait(100);
            }
            draw_sprite(double_buffer, dudes, 106, 64);
            kqDraw.blit2screen(0, 0);
            kq_wait(1000);
            delete (staff);
            delete (dudes);
            delete (tdudes);
            /*
             *  TODO: this fade should actually be to white
             *  if (_color_depth == 8)
             *  fade_from (pal, whp, 1);
             *  else
             */
            kqDraw.do_transition(TRANS_FADE_WHITE, 1);
        }
        clear_to_color(double_buffer, 15);
        kqDraw.blit2screen(0, 0);
        set_palette(pal);

        for (fade_color = 0; fade_color < 16; fade_color++)
        {
            clear_to_color(double_buffer, 15 - fade_color);
            masked_blit(title, double_buffer, 0, 0, 0, 60 - (fade_color * 4), KQ_SCREEN_W, 124);
            kqDraw.blit2screen(0, 0);
            kq_wait(fade_color == 0 ? 500 : 100);
        }
        if (skip_splash == 0)
        {
            kq_wait(500);
        }
#ifdef DEBUGMODE
    }
    else
    {
        set_palette(pal);
    }
#endif
    Game.reset_world();

    /* Draw menu and handle menu selection */
    while (!stop)
    {
        if (redraw)
        {
            clear_bitmap(double_buffer);
            masked_blit(title, double_buffer, 0, 0, 0, 0, KQ_SCREEN_W, 124);
            kqDraw.menubox(double_buffer, 112, 116, 10, 4, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
            kqDraw.print_font(double_buffer, 128, 124, _("Continue"), eFontColor::FONTCOLOR_NORMAL);
            kqDraw.print_font(double_buffer, 128, 132, _("New Game"), eFontColor::FONTCOLOR_NORMAL);
            kqDraw.print_font(double_buffer, 136, 140, _("Config"), eFontColor::FONTCOLOR_NORMAL);
            kqDraw.print_font(double_buffer, 144, 148, _("Exit"), eFontColor::FONTCOLOR_NORMAL);
            draw_sprite(double_buffer, menuptr, 112, ptr * 8 + 124);
            redraw = 0;
        }
        display_credits(double_buffer);
        kqDraw.blit2screen(0, 0);
        PlayerInput.readcontrols();
        if (PlayerInput.bhelp)
        {
            Game.unpress();
            show_help();
            redraw = 1;
        }
        if (PlayerInput.up)
        {
            Game.unpress();
            if (ptr > 0)
            {
                ptr--;
            }
            else
            {
                ptr = 3;
            }
            play_effect(SND_CLICK, 128);
            redraw = 1;
        }
        if (PlayerInput.down)
        {
            Game.unpress();
            if (ptr < 3)
            {
                ptr++;
            }
            else
            {
                ptr = 0;
            }
            play_effect(SND_CLICK, 128);
            redraw = 1;
        }
        if (PlayerInput.balt)
        {
            Game.unpress();
            if (ptr == 0) /* User selected "Continue" */
            {
                // Check if we've saved any games at all
                bool anyslots = false;
                for (auto& sg : savegame)
                {
                    if (sg.num_characters > 0)
                    {
                        anyslots = true;
                        break;
                    }
                }
                if (!anyslots)
                {
                    stop = 2;
                }
                else if (saveload(0) == 1)
                {
                    stop = 1;
                }
                redraw = 1;
            }
            else if (ptr == 1)   /* User selected "New Game" */
            {
                stop = 2;
            }
            else if (ptr == 2)   /* Config */
            {
                clear_bitmap(double_buffer);
                config_menu();
                redraw = 1;

                /* TODO: Save Global Settings Here */
            }
            else if (ptr == 3)   /* Exit */
            {
                Game.klog(_("Then exit you shall!"));
                return 2;
            }
        }
    }
    if (stop == 2)
    {
        /* New game init */
        extern int load_game_xml(const char* filename);
        load_game_xml(kqres(eDirectories::DATA_DIR, "starting.xml").c_str());
    }
    return stop - 1;
}

int KSaveGame::system_menu()
{
    int stop = 0, ptr = 0;
    char save_str[10];
    eFontColor text_color = eFontColor::FONTCOLOR_NORMAL;

    strcpy(save_str, _("Save  "));

    if (cansave == 0)
    {
        text_color = eFontColor::FONTCOLOR_DARK;
#ifdef KQ_CHEATS
        if (hasCheatEnabled)
        {
            strcpy(save_str, _("[Save]"));
            text_color = eFontColor::FONTCOLOR_NORMAL;
        }
#endif /* KQ_CHEATS */
    }

    while (!stop)
    {
        Game.do_check_animation();
        kqDraw.drawmap();
        kqDraw.menubox(double_buffer, xofs, yofs, 8, 4, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);

        kqDraw.print_font(double_buffer, 16 + xofs, 8 + yofs, save_str, text_color);
        kqDraw.print_font(double_buffer, 16 + xofs, 16 + yofs, _("Load"), eFontColor::FONTCOLOR_NORMAL);
        kqDraw.print_font(double_buffer, 16 + xofs, 24 + yofs, _("Config"), eFontColor::FONTCOLOR_NORMAL);
        kqDraw.print_font(double_buffer, 16 + xofs, 32 + yofs, _("Exit"), eFontColor::FONTCOLOR_NORMAL);

        draw_sprite(double_buffer, menuptr, 0 + xofs, ptr * 8 + 8 + yofs);
        kqDraw.blit2screen(xofs, yofs);
        PlayerInput.readcontrols();

        // TT:
        // When pressed, 'up' or 'down' == 1.  Otherwise, they equal 0.  So:
        //    ptr = ptr - up + down;
        // will correctly modify the pointer, but with less code.
        if (PlayerInput.up || PlayerInput.down)
        {
            ptr = ptr + PlayerInput.up - PlayerInput.down;
            if (ptr < 0)
            {
                ptr = 3;
            }
            else if (ptr > 3)
            {
                ptr = 0;
            }
            play_effect(SND_CLICK, 128);
            Game.unpress();
        }

        if (PlayerInput.balt)
        {
            Game.unpress();

            if (ptr == 0)
            {
// Pointer is over the SAVE option
#ifdef KQ_CHEATS
                if (cansave == 1 || hasCheatEnabled)
#else
                if (cansave == 1)
#endif /* KQ_CHEATS */
                {
                    saveload(1);
                    stop = 1;
                }
                else
                {
                    play_effect(SND_BAD, 128);
                }
            }

            if (ptr == 1)
            {
                if (saveload(0) != 0)
                {
                    stop = 1;
                }
            }

            if (ptr == 2)
            {
                config_menu();
            }

            if (ptr == 3)
            {
                return confirm_quit();
            }
        }

        if (PlayerInput.bctrl)
        {
            stop = 1;
            Game.unpress();
        }
    }

    return 0;
}

KSaveGame SaveGame;
