/*! \file
 * \brief Setup and menu code
 *
 * \author JB
 * \date ??????
 * \remark Updated  ML Oct-2002
 */

#include <stdio.h>
#include <string>

#include "combat.h"
#include "constants.h"
#include "draw.h"
#include "gfx.h"
#include "input.h"
#include "kq.h"
#include "music.h"
#include "platform.h"
#include "res.h"
#include "setup.h"
#include "timing.h"

/*! \name Globals */

/*! Debug level 0..3 */
char debugging = 0;

/*! Speed-up for slower machines */
char slow_computer = 0;

bool isWindowed = true;

/*  Internal variables  */
static DATAFILE* sfx[MAX_SAMPLES];

/*  Internal functions  */
static int load_samples();
static int getavalue(const char*, int, int, int, bool, void (*)(int));
static int getakey();
static void parse_allegro_setup();
static void parse_jb_setup();

/*! \brief Play sound effects / music if adjusting it */
static void sound_feedback(int val)
{
    set_volume(val * 10, 0);
    play_effect(1, 127);
}
static void music_feedback(int val)
{
    Music.set_music_volume(float(val * 10) / 250.0f);
}

/*! \brief Draw a setting and its title
 *
 * Helper function for the config menu.
 * The setting title is drawn, then its value right-aligned.
 * \author PH
 * \date 20030527
 * \param   y y-coord of line
 * \param   caption Title of the setting (e.g. "Windowed mode:")
 * \param   value The setting (e.g. "Yes")
 * \param   color The foreground color of the text
 */
static void citem(int y, const char* caption, const char* value, eFontColor color)
{
    kqDraw.print_font(double_buffer, 48 + xofs, y + yofs, caption, color);
    kqDraw.print_font(double_buffer, SCREEN_H2 - 8 * strlen(value) + xofs, y + yofs, value, color);
}

/*! \brief Display configuration menu
 *
 * This is the config menu that is called from the system
 * menu.  Here you can adjust the music or sound volume, or
 * the speed that the battle gauge moves at.
 */
void config_menu()
{
    size_t stop = 0, ptr = 0;
    int p;
    eFontColor fontColor;
    int temp_key = 0;

#ifdef DEBUGMODE
#define MENU_SIZE 18
#else
#define MENU_SIZE 17
#endif
    static const char* dc[MENU_SIZE];

    /* Define rows with appropriate spacings for breaks between groups */
    int row[MENU_SIZE];

    for (p = 0; p < 4; p++)
    {
        row[p] = (p + 4) * 8; // (p * 8) + 32
    }
    for (p = 4; p < 12; p++)
    {
        row[p] = (p + 5) * 8; // (p * 8) + 40
    }
    for (p = 12; p < 15; p++)
    {
        row[p] = (p + 6) * 8; // (p * 8) + 48
    }
    for (p = 15; p < MENU_SIZE; p++)
    {
        row[p] = (p + 7) * 8; // (p * 8) + 56
    }

    /* Helper strings */
    dc[0] = _("Display KQ in a window.");
    dc[1] = _("Stretch to fit 640x480 resolution.");
    dc[2] = _("Display the frame rate during play.");
    dc[3] = _("Wait for vertical retrace.");
    dc[4] = _("Key used to move up.");
    dc[5] = _("Key used to move down.");
    dc[6] = _("Key used to move left.");
    dc[7] = _("Key used to move right.");
    dc[8] = _("Key used to confirm action.");
    dc[9] = _("Key used to cancel action.");
    dc[10] = _("Key used to call character menu.");
    dc[11] = _("Key used to call system menu.");
    dc[12] = _("Toggle sound and music on/off.");
    dc[13] = _("Overall sound volume (affects music).");
    dc[14] = _("Music volume.");
    dc[15] = _("Animation speed-ups for slow machines.");
    dc[16] = _("Toggle how to allocate CPU usage.");
#ifdef DEBUGMODE
    dc[17] = _("Things you can do only in DebugMode.");
#endif

    Game.unpress();
    push_config_state();
    set_config_file(kqres(SETTINGS_DIR, "kq.cfg").c_str());
    while (!stop)
    {
        Game.do_check_animation();
        kqDraw.drawmap();
        kqDraw.menubox(double_buffer, 88 + xofs, yofs, 16, 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        kqDraw.print_font(double_buffer, 96 + xofs, 8 + yofs, _("KQ Configuration"), eFontColor::FONTCOLOR_GOLD);
        kqDraw.menubox(double_buffer, 32 + xofs, 24 + yofs, 30, MENU_SIZE + 3, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);

        citem(row[0], _("Windowed mode:"), isWindowed ? _("YES") : _("NO"), eFontColor::FONTCOLOR_NORMAL);
        citem(row[1], _("Stretch Display:"), stretch_view == 1 ? _("YES") : _("NO"), eFontColor::FONTCOLOR_NORMAL);
        citem(row[2], _("Show Frame Rate:"), show_frate == 1 ? _("YES") : _("NO"), eFontColor::FONTCOLOR_NORMAL);
        citem(row[3], _("Wait for Retrace:"), wait_retrace == 1 ? _("YES") : _("NO"), eFontColor::FONTCOLOR_NORMAL);
        citem(row[4], _("Up Key:"), kq_keyname(PlayerInput.kup), eFontColor::FONTCOLOR_NORMAL);
        citem(row[5], _("Down Key:"), kq_keyname(PlayerInput.kdown), eFontColor::FONTCOLOR_NORMAL);
        citem(row[6], _("Left Key:"), kq_keyname(PlayerInput.kleft), eFontColor::FONTCOLOR_NORMAL);
        citem(row[7], _("Right Key:"), kq_keyname(PlayerInput.kright), eFontColor::FONTCOLOR_NORMAL);
        citem(row[8], _("Confirm Key:"), kq_keyname(PlayerInput.kalt), eFontColor::FONTCOLOR_NORMAL);
        citem(row[9], _("Cancel Key:"), kq_keyname(PlayerInput.kctrl), eFontColor::FONTCOLOR_NORMAL);
        citem(row[10], _("Menu Key:"), kq_keyname(PlayerInput.kenter), eFontColor::FONTCOLOR_NORMAL);
        citem(row[11], _("System Menu Key:"), kq_keyname(PlayerInput.kesc), eFontColor::FONTCOLOR_NORMAL);
        citem(row[12], _("Sound System:"), is_sound ? _("ON") : _("OFF"), eFontColor::FONTCOLOR_NORMAL);

        fontColor = eFontColor::FONTCOLOR_NORMAL;
        /* TT: This needs to check for ==0 because 1 means sound init */
        if (is_sound == 0)
        {
            fontColor = eFontColor::FONTCOLOR_DARK;
        }

        sprintf(strbuf, "%3d%%", gsvol * 100 / 250);
        citem(row[13], _("Sound Volume:"), strbuf, fontColor);

        sprintf(strbuf, "%3d%%", gmvol * 100 / 250);
        citem(row[14], _("Music Volume:"), strbuf, fontColor);

        citem(row[15], _("Slow Computer:"), slow_computer ? _("YES") : _("NO"), eFontColor::FONTCOLOR_NORMAL);

        if (cpu_usage)
        {
            sprintf(strbuf, _("rest(%d)"), cpu_usage - 1);
        }
        else
        {
            sprintf(strbuf, "yield_timeslice()");
        }
        citem(row[16], _("CPU Usage:"), strbuf, eFontColor::FONTCOLOR_NORMAL);

#ifdef DEBUGMODE
        if (debugging)
        {
            sprintf(strbuf, "%d", debugging);
        }
        citem(row[17], _("DebugMode Stuff:"), debugging ? strbuf : _("OFF"), eFontColor::FONTCOLOR_NORMAL);
#endif

        /* This affects the VISUAL placement of the arrow */
        p = ptr;
        if (ptr > 3)
        {
            p++;
        }
        if (ptr > 11)
        {
            p++;
        }
        if (ptr > 14)
        {
            p++;
        }
        draw_sprite(double_buffer, menuptr, 32 + xofs, p * 8 + 32 + yofs);

        /* This is the bottom window, where the description goes */
        kqDraw.menubox(double_buffer, xofs, 216 + yofs, 38, 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        kqDraw.print_font(double_buffer, 8 + xofs, 224 + yofs, dc[ptr], eFontColor::FONTCOLOR_NORMAL);
        kqDraw.blit2screen(xofs, yofs);

        PlayerInput.readcontrols();
        if (PlayerInput.up)
        {
            Game.unpress();
            // "jump" over unusable options
            if (ptr == 15 && is_sound == 0)
            {
                ptr -= 2;
            }
            if (ptr > 0)
            {
                ptr--;
            }
            else
            {
                ptr = MENU_SIZE - 1;
            }
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.down)
        {
            Game.unpress();
            // "jump" over unusable options
            if (ptr == 12 && is_sound == 0)
            {
                ptr += 2;
            }
            if (ptr < MENU_SIZE - 1)
            {
                ptr++;
            }
            else
            {
                ptr = 0;
            }
            play_effect(SND_CLICK, 128);
        }
        if (PlayerInput.balt)
        {
            Game.unpress();
            switch (ptr)
            {
            case 0:
                kqDraw.text_ex(eBubbleStyle::BUBBLE_TEXT, 255, _("Changing the display mode to or from windowed "
                                       "view could have serious ramifications. It is "
                                       "advised that you save first."));
                if (!isWindowed)
                {
                    sprintf(strbuf, _("Switch to windowed mode?"));
                }
                else
                {
                    sprintf(strbuf, _("Switch to full screen?"));
                }
                p = kqDraw.prompt(255, 2, eBubbleStyle::BUBBLE_TEXT, strbuf, _("  no"), _("  yes"), "");
                if (p == 1)
                {
                    isWindowed = !isWindowed;
                    set_config_int(nullptr, "windowed", isWindowed ? 1 : 0);
                    set_graphics_mode();
                }
                break;
            case 1:
                kqDraw.text_ex(eBubbleStyle::BUBBLE_TEXT, 255, _("Changing the stretched view option could have "
                                       "serious ramifications. It is advised that you "
                                       "save your game before trying this."));
                if (stretch_view == 0)
                {
                    sprintf(strbuf, _("Try to stretch the display?"));
                }
                else
                {
                    sprintf(strbuf, _("Switch to unstretched display?"));
                }
                p = kqDraw.prompt(255, 2, eBubbleStyle::BUBBLE_TEXT, strbuf, _("  no"), _("  yes"), "");
                if (p == 1)
                {
                    stretch_view = !stretch_view;
                    set_config_int(nullptr, "stretch_view", stretch_view);
                    set_graphics_mode();
                }
                break;
            case 2:
                show_frate = !show_frate;
                set_config_int(nullptr, "show_frate", show_frate);
                break;
            case 3:
                wait_retrace = !wait_retrace;
                set_config_int(nullptr, "wait_retrace", wait_retrace);
                break;
            case 4:
                while ((temp_key = getakey()) == 0)
                {
                }
                PlayerInput.kup = temp_key;
                Game.unpress();
                temp_key = 0;
                set_config_int(nullptr, "kup", PlayerInput.kup);
                break;
            case 5:
                while ((temp_key = getakey()) == 0)
                {
                }
                PlayerInput.kdown = temp_key;
                Game.unpress();
                temp_key = 0;
                set_config_int(nullptr, "kdown", PlayerInput.kdown);
                break;
            case 6:
                while ((temp_key = getakey()) == 0)
                {
                }
                PlayerInput.kleft = temp_key;
                Game.unpress();
                temp_key = 0;
                set_config_int(nullptr, "kleft", PlayerInput.kleft);
                break;
            case 7:
                while ((temp_key = getakey()) == 0)
                {
                }
                PlayerInput.kright = temp_key;
                Game.unpress();
                temp_key = 0;
                set_config_int(nullptr, "kright", PlayerInput.kright);
                break;
            case 8:
                while ((temp_key = getakey()) == 0)
                {
                }
                PlayerInput.kalt = temp_key;
                Game.unpress();
                temp_key = 0;
                set_config_int(nullptr, "kalt", PlayerInput.kalt);
                break;
            case 9:
                while ((temp_key = getakey()) == 0)
                {
                }
                PlayerInput.kctrl = temp_key;
                Game.unpress();
                temp_key = 0;
                set_config_int(nullptr, "kctrl", PlayerInput.kctrl);
                break;
            case 10:
                while ((temp_key = getakey()) == 0)
                {
                }
                PlayerInput.kenter = temp_key;
                Game.unpress();
                temp_key = 0;
                set_config_int(nullptr, "kenter", PlayerInput.kenter);
                break;
            case 11:
                while ((temp_key = getakey()) == 0)
                {
                }
                PlayerInput.kesc = temp_key;
                Game.unpress();
                temp_key = 0;
                set_config_int(nullptr, "kesc", PlayerInput.kesc);
                break;
            case 12:
                if (is_sound == 2)
                {
                    sound_init();
                }
                else
                {
                    if (is_sound == 0)
                    {
                        is_sound = 1;
                        kqDraw.print_font(double_buffer, 92 + 2 + xofs, 204 + yofs, _("...please wait..."), eFontColor::FONTCOLOR_NORMAL);
                        kqDraw.blit2screen(xofs, yofs);
                        sound_init();
                        Music.play_music(g_map.song_file, 0);
                    }
                }
                set_config_int(nullptr, "is_sound", is_sound != 0);
                break;
            case 13:
                if (is_sound == 2)
                {
                    p = getavalue(_("Sound Volume"), 0, 25, gsvol / 10, true, sound_feedback);
                    if (p != -1)
                    {
                        gsvol = p * 10;
                    }

                    /* make sure to set it no matter what */
                    set_volume(gsvol, 0);
                    set_config_int(nullptr, "gsvol", gsvol);
                }
                else
                    /* Not as daft as it seems, SND_BAD also wobbles the screen */
                {
                    play_effect(SND_BAD, 128);
                }
                break;
            case 14:
                if (is_sound == 2)
                {
                    p = getavalue(_("Music Volume"), 0, 25, gmvol / 10, true, music_feedback);
                    if (p != -1)
                    {
                        gmvol = p * 10;
                    }

                    /* make sure to set it no matter what */
                    Music.set_music_volume(gmvol / 250.0);
                    set_config_int(nullptr, "gmvol", gmvol);
                }
                else
                {
                    play_effect(SND_BAD, 128);
                }
                break;
            case 15:
                /* TT: toggle slow_computer */
                slow_computer = !slow_computer;
                set_config_int(nullptr, "slow_computer", slow_computer);
                break;
            case 16:
                /* TT: Adjust the CPU usage:yield_timeslice() or rest() */
                cpu_usage++;
                if (cpu_usage > 2)
                {
                    cpu_usage = 0;
                }
                break;
#ifdef DEBUGMODE
            case 17:
                /* TT: Things we only have access to when we're in debug mode */
                if (debugging < 4)
                {
                    debugging++;
                }
                else
                {
                    debugging = 0;
                }
                break;
#endif
            }
        }
        if (PlayerInput.bctrl)
        {
            Game.unpress();
            stop = 1;
        }
    }
    pop_config_state();
}

/*! \brief Release memory used by samples
 * \author  : Josh Bolduc
 * \date ????????
 *
 *  Duh.
 */
void free_samples()
{
    size_t index;

    if (is_sound == 0)
    {
        return;
    }

    for (index = 0; index < MAX_SAMPLES; index++)
    {
        unload_datafile_object(sfx[index]);
    }
}

/*! \brief Process keypresses when mapping new keys
 *
 * This grabs whatever key is being pressed and returns it to the caller.
 * PH 20030527 Removed call to keypressed() and added poll_music()
 *
 * \returns the key being pressed, 0 if error (or cancel?)
 */
static int getakey()
{
    int a;

    clear_keybuf();
    kqDraw.menubox(double_buffer, 108 + xofs, 108 + yofs, 11, 1, eMenuBoxColor::DARKBLUE);
    kqDraw.print_font(double_buffer, 116 + xofs, 116 + yofs, _("Press a key"), eFontColor::FONTCOLOR_NORMAL);
    kqDraw.blit2screen(xofs, yofs);

    while (1)
    {
        Music.poll_music();
        for (a = 0; a < KEY_MAX; a++)
        {
            if (key[a] != 0)
            {
                return a;
            }
        }
    }
    return 0;
}

/*! \brief Get value for option
 *
 * Display a bar and allow the user to adjust between fixed limits.
 * You can specify a feedback function with the signature
 * void (*fb)(int)
 * which is called whenever the value changes.
 *
 * \param   capt Caption
 * \param   minu Minimum value of option
 * \param   maxu Maximum vlaue of option
 * \param   cv Current value (initial value)
 * \param   sp Show percent. If sp is true, show as a percentage of maxu
 * \param   fb Feedback function (or NULL for no feedback)
 * \returns the new value for option, or -1 if cancelled.
 */
static int getavalue(const char* capt, int minu, int maxu, int cv, bool sp, void (*fb)(int))
{
    if (maxu <= 0 || maxu >= 40)
    {
        return -1;
    }
    bool stop = false;
    while (!stop)
    {
        Game.do_check_animation();
        kqDraw.menubox(double_buffer, 148 - (maxu * 4) + xofs, 100 + yofs, maxu + 1, 3, eMenuBoxColor::DARKBLUE);
        kqDraw.print_font(double_buffer, 160 - (strlen(capt) * 4) + xofs, 108 + yofs, capt, eFontColor::FONTCOLOR_GOLD);
        kqDraw.print_font(double_buffer, 152 - (maxu * 4) + xofs, 116 + yofs, "<", eFontColor::FONTCOLOR_NORMAL);
        kqDraw.print_font(double_buffer, 160 + (maxu * 4) + xofs, 116 + yofs, ">", eFontColor::FONTCOLOR_NORMAL);
        int b = 160 - (maxu * 4) + xofs;
        for (int a = 0; a < cv; a++)
        {
            rectfill(double_buffer, a * 8 + b + 1, 117 + yofs, a * 8 + b + 7, 123 + yofs, 50);
            rectfill(double_buffer, a * 8 + b, 116 + yofs, a * 8 + b + 6, 122 + yofs, 21);
        }
        char strbuf[10];
        if (sp)
        {
            sprintf(strbuf, "%d%%", cv * 100 / maxu);
        }
        else
        {
            sprintf(strbuf, "%d", cv);
        }
        kqDraw.print_font(double_buffer, 160 - (strlen(strbuf) * 4) + xofs, 124 + yofs, strbuf, eFontColor::FONTCOLOR_GOLD);
        kqDraw.blit2screen(xofs, yofs);

        PlayerInput.readcontrols();
        if (PlayerInput.left)
        {
            Game.unpress();
            cv--;
            if (cv < minu)
            {
                cv = minu;
            }
            if (fb)
            {
                fb(cv);
            }
        }
        if (PlayerInput.right)
        {
            Game.unpress();
            cv++;
            if (cv > maxu)
            {
                cv = maxu;
            }
            if (fb)
            {
                fb(cv);
            }
        }
        if (PlayerInput.balt)
        {
            Game.unpress();
            stop = true;
        }
        if (PlayerInput.bctrl)
        {
            Game.unpress();
            return -1;
        }
    }
    return cv;
}

#if (ALLEGRO_VERSION >= 4 && ALLEGRO_SUB_VERSION >= 2)
// TT: Already defined in setup.h
// #define kq_keyname scancode_to_name
#else
const char* kq_keyname(int scancode)
{
    if (scancode >= 0 && scancode < (signed)N_KEYNAMES)
    {
        return keynames[scancode];
    }
    else
    {
        return "???";
    }
}
#endif

/*! \brief Load sample files
 * \author JB
 * \date ????????
 *
 * Load the list of samples from the data file.
 *
 * \todo RB FIXME: This must be generated from the kqsnd.h header,
 *          not hardcoded, to make it easier to maintain (a perl script?).
 * \remark Updated : 20020914 - 05:20 (RB)
 * \remark ML 2002-09-22: altered this so it returns an error on failure
 *
 * \returns 0 on success, 1 on failure.
 */
static int load_samples()
{
    AL_CONST char* sndfiles[MAX_SAMPLES] =
    {
        "WHOOSH_WAV",   "MENUMOVE_WAV", "BAD_WAV",     "ITEM_WAV",
        "EQUIP_WAV",    "DEEQUIP_WAV",  "BUYSELL_WAV", "TWINKLE_WAV",
        "SCORCH_WAV",   "POISON_WAV",   "CHOP_WAV",    "SLASH_WAV",
        "STAB_WAV",     "HIT_WAV",      "ICE_WAV",     "WIND_WAV",
        "QUAKE_WAV",    "BLACK_WAV",    "WHITE_WAV",   "BOLT1_WAV",
        "FLOOD_WAV",    "HURT_WAV",     "BMAGIC_WAV",  "SHIELD_WAV",
        "KILL_WAV",     "DOOROPEN_WAV", "DOOR2_WAV",   "STAIRS_WAV",
        "TELEPORT_WAV", "CURE_WAV",     "RECOVER_WAV", "ARROW_WAV",
        "BOLT2_WAV",    "BOLT3_WAV",    "FLAME_WAV",   "BLIND_WAV",
        "INN_WAV",      "CONFUSE_WAV",  "DISPEL_WAV",  "DOOM_WAV",
        "DRAIN_WAV",    "GAS_WAV",      "EXPLODE_WAV"
    };
    size_t index;

    if (is_sound == 0)
    {
        return 1;
    }
    std::string sound_datafile(kqres(DATA_DIR, "kqsnd.dat"));
    for (index = 0; index < MAX_SAMPLES; index++)
    {
        sfx[index] = load_datafile_object(sound_datafile.c_str(), sndfiles[index]);
        if (sfx[index] == nullptr)
        {
            sprintf(strbuf, _("Error loading .WAV file: %s.\n"), sndfiles[index]);
            Game.klog(strbuf);
            return 1;
        }
    }
    return 0;
}

/*! \brief Parse allegro file kq.cfg
 *
 * This is like parse_setup(), but using Allegro format files
 *
 * \author PH
 * \date 20030831
 */
static void parse_allegro_setup()
{
    std::string cfg = kqres(SETTINGS_DIR, "kq.cfg").c_str();

    if (!exists(cfg.c_str()))
    {
        /* config file does not exist. Fall back to setup.cfg */
        /* Transitional code */
        parse_jb_setup();
        push_config_state();
        set_config_file(kqres(SETTINGS_DIR, "kq.cfg").c_str());

        set_config_int(nullptr, "skip_intro", skip_intro);
        set_config_int(nullptr, "windowed", isWindowed ? 1 : 0);

        set_config_int(nullptr, "stretch_view", stretch_view);
        set_config_int(nullptr, "show_frate", show_frate);
        set_config_int(nullptr, "is_sound", is_sound);
        set_config_int(nullptr, "use_joy", use_joy);
        set_config_int(nullptr, "slow_computer", slow_computer);

        set_config_int(nullptr, "kup", PlayerInput.kup);
        set_config_int(nullptr, "kdown", PlayerInput.kdown);
        set_config_int(nullptr, "kleft", PlayerInput.kleft);
        set_config_int(nullptr, "kright", PlayerInput.kright);
        set_config_int(nullptr, "kesc", PlayerInput.kesc);
        set_config_int(nullptr, "kalt", PlayerInput.kalt);
        set_config_int(nullptr, "kctrl", PlayerInput.kctrl);
        set_config_int(nullptr, "kenter", PlayerInput.kenter);

#ifdef DEBUGMODE
        set_config_int(nullptr, "debugging", debugging);
#endif

        pop_config_state();
        return;
    }
    push_config_state();
    set_config_file(cfg.c_str());

    /* NB. JB's config file uses intro=yes --> skip_intro=0 */
    skip_intro = get_config_int(nullptr, "skip_intro", 0);
    isWindowed = get_config_int(nullptr, "windowed", 1) != 0;
    stretch_view = get_config_int(nullptr, "stretch_view", 1);
    wait_retrace = get_config_int(nullptr, "wait_retrace", 1);
    show_frate = get_config_int(nullptr, "show_frate", 0);
    is_sound = get_config_int(nullptr, "is_sound", 1);
    gmvol = get_config_int(nullptr, "gmvol", 250);
    gsvol = get_config_int(nullptr, "gsvol", 250);
    use_joy = get_config_int(nullptr, "use_joy", 0);
    slow_computer = get_config_int(nullptr, "slow_computer", 0);
    cpu_usage = get_config_int(nullptr, "cpu_usage", 2);
#ifdef KQ_CHEATS
    hasCheatEnabled = (get_config_int(nullptr, "cheat", 0) != 0);
    no_random_encounters = get_config_int(nullptr, "no_random_encounters", 0);
    no_monsters = get_config_int(nullptr, "no_monsters", 0);
    every_hit_999 = get_config_int(nullptr, "every_hit_999", 0);
#endif
#ifdef DEBUGMODE
    debugging = get_config_int(nullptr, "debugging", 0);
#endif

    PlayerInput.kup = get_config_int(nullptr, "kup", KEY_UP);
    PlayerInput.kdown = get_config_int(nullptr, "kdown", KEY_DOWN);
    PlayerInput.kleft = get_config_int(nullptr, "kleft", KEY_LEFT);
    PlayerInput.kright = get_config_int(nullptr, "kright", KEY_RIGHT);
    PlayerInput.kesc = get_config_int(nullptr, "kesc", KEY_ESC);
    PlayerInput.kalt = get_config_int(nullptr, "kalt", KEY_ALT);
    PlayerInput.kctrl = get_config_int(nullptr, "kctrl", KEY_LCONTROL);
    PlayerInput.kenter = get_config_int(nullptr, "kenter", KEY_ENTER);
    pop_config_state();
}

/*! \brief Parse setup.cfg
 *
 * Read settings from file
 * Parse the setup.cfg file for key configurations.
 * This file would also contain sound options, but that
 * isn't necessary right now.
 *
 * Remember that setup.cfg is found in the /saves dir!
 */
static void parse_jb_setup()
{
    FILE* s = nullptr;
    int dab = 0;

    /* Default key assignments */
    PlayerInput.kup = KEY_UP;
    PlayerInput.kdown = KEY_DOWN;
    PlayerInput.kright = KEY_RIGHT;
    PlayerInput.kleft = KEY_LEFT;
    PlayerInput.kalt = KEY_ALT;
    PlayerInput.kctrl = KEY_LCONTROL;
    PlayerInput.kenter = KEY_ENTER;
    PlayerInput.kesc = KEY_ESC;
    PlayerInput.jbalt = 0;
    PlayerInput.jbctrl = 1;
    PlayerInput.jbenter = 2;
    PlayerInput.jbesc = 3;
    /* PH Why in the world doesn't he use Allegro cfg functions here? */
    if (!(s = fopen(kqres(SETTINGS_DIR, "setup.cfg").c_str(), "r")))
    {
        Game.klog(_("Could not open saves/setup.cfg - Using defaults."));
        return;
    }
    fscanf(s, "%s", strbuf);
    while (!feof(s))
    {
        if (strbuf[0] == '#')
        {
            fgets(strbuf, 254, s);
        }
#ifdef KQ_CHEATS
        if (!strcmp(strbuf, "cheat"))
        {
            fscanf(s, "%d", &dab);
            hasCheatEnabled = (dab != 0);
        }
#endif
        if (!strcmp(strbuf, "debug"))
        {
            fscanf(s, "%d", &dab);
            debugging = dab;
        }
        if (!strcmp(strbuf, "intro"))
        {
            fscanf(s, "%s", strbuf);
            if (!strcmp(strbuf, "no"))
            {
                skip_intro = 1;
            }
        }
        if (!strcmp(strbuf, "windowed"))
        {
            fscanf(s, "%s", strbuf);
            if (!strcmp(strbuf, "yes"))
            {
                isWindowed = true;
            }
        }
        if (!strcmp(strbuf, "stretch"))
        {
            fscanf(s, "%s", strbuf);
            if (!strcmp(strbuf, "yes"))
            {
                stretch_view = 1;
            }
        }
        if (!strcmp(strbuf, "framerate"))
        {
            fscanf(s, "%s", strbuf);
            if (!strcmp(strbuf, "on"))
            {
                show_frate = 1;
            }
        }
        if (!strcmp(strbuf, "sound"))
        {
            fscanf(s, "%s", strbuf);
            if (!strcmp(strbuf, "off"))
            {
                is_sound = 0;
            }
        }
        if (!strcmp(strbuf, "joystick"))
        {
            fscanf(s, "%s", strbuf);
            if (!strcmp(strbuf, "no"))
            {
                use_joy = 0;
            }
        }
        if (!strcmp(strbuf, "slow_computer"))
        {
            fscanf(s, "%s", strbuf);
            if (!strcmp(strbuf, "yes"))
            {
                slow_computer = 1;
            }
        }
        if (!strcmp(strbuf, "rightkey"))
        {
            fscanf(s, "%s", strbuf);
            PlayerInput.kright = atoi(strbuf);
        }
        if (!strcmp(strbuf, "leftkey"))
        {
            fscanf(s, "%s", strbuf);
            PlayerInput.kleft = atoi(strbuf);
        }
        if (!strcmp(strbuf, "upkey"))
        {
            fscanf(s, "%s", strbuf);
            PlayerInput.kup = atoi(strbuf);
        }
        if (!strcmp(strbuf, "downkey"))
        {
            fscanf(s, "%s", strbuf);
            PlayerInput.kdown = atoi(strbuf);
        }
        if (!strcmp(strbuf, "sysmenukey"))
        {
            fscanf(s, "%s", strbuf);
            PlayerInput.kesc = atoi(strbuf);
        }
        if (!strcmp(strbuf, "cancelkey"))
        {
            fscanf(s, "%s", strbuf);
            PlayerInput.kctrl = atoi(strbuf);
        }
        if (!strcmp(strbuf, "confirmkey"))
        {
            fscanf(s, "%s", strbuf);
            PlayerInput.kalt = atoi(strbuf);
        }
        if (!strcmp(strbuf, "chrmenukey"))
        {
            fscanf(s, "%s", strbuf);
            PlayerInput.kenter = atoi(strbuf);
        }
        fscanf(s, "%s", strbuf);
    }
    fclose(s);
}

/*! \brief Parse setup file
 *
 * \date 20030831
 * \author PH
 */
void parse_setup() { parse_allegro_setup(); }

/*! \brief Play sample effect
 *
 * Play an effect... if possible/necessary.  If the effect to
 * be played is the 'bad-move' effect, than do something visually
 * so that even if sound is off you know you did something bad :)
 * PH added explode effect.
 *
 * \param   efc Effect to play (index in sfx[])
 * \param   panning Left/right pan - see Allegro's play_sample()
 */
void play_effect(int efc, int panning)
{
    int a, s, xo = 1, yo = 1;
    static const int bx[8] = {-1, 0, 1, 0, -1, 0, 1, 0};
    static const int by[8] = {-1, 0, 1, 0, 1, 0, -1, 0};
    static const int sc[] = {1, 2, 3, 5, 3, 3, 3, 2, 1};
    SAMPLE* samp = nullptr;
    PALETTE whiteout, old;

    /* Patch provided by mattrope: */
    /* sfx array is empty if sound is not initialized */
    if (is_sound != 0)
    {
        samp = (SAMPLE*)sfx[efc]->dat;
    }
    else
    {
        samp = nullptr; /* PH not strictly needed but I added it */
    }

    switch (efc)
    {
    default:
        if (samp)
        {
            play_sample(samp, gsvol, panning, 1000, 0);
        }
        break;
    case SND_BAD:
        fullblit(double_buffer, fx_buffer);

        if (samp)
        {
            play_sample(samp, gsvol, panning, 1000, 0);
        }
        clear_bitmap(double_buffer);
        blit(fx_buffer, double_buffer, xofs, yofs, xofs, yofs, KQ_SCREEN_W, KQ_SCREEN_H);

        if (in_combat == 0)
        {
            xo = xofs;
            yo = yofs;
        }

        for (a = 0; a < 8; a++)
        {
            kqDraw.blit2screen(xo + bx[a], yo + by[a]);
            kq_wait(10);
        }
        fullblit(fx_buffer, double_buffer);
        break;
    case SND_EXPLODE:
        fullblit(double_buffer, fx_buffer);
        clear_bitmap(double_buffer);
        get_palette(old);
        for (a = 0; a < 256; ++a)
        {
            s = (old[a].r + old[a].g + old[a].b) > 40 ? 0 : 63;
            whiteout[a].r = whiteout[a].g = whiteout[a].b = s;
        }
        blit(fx_buffer, double_buffer, xofs, yofs, xofs, yofs, KQ_SCREEN_W, KQ_SCREEN_H);
        if (samp)
        {
            play_sample(samp, gsvol, panning, 1000, 0);
        }
        for (s = 0; s < (int)(sizeof(sc) / sizeof(*sc)); ++s)
        {
            if (s == 1)
            {
                set_palette(whiteout);
            }
            if (s == 6)
            {
                set_palette(old);
            }

            for (a = 0; a < 8; a++)
            {
                kqDraw.blit2screen(xofs + bx[a] * sc[s], yofs + by[a] * sc[s]);
                kq_wait(10);
            }
        }
        fullblit(fx_buffer, double_buffer);
        break;
    }
}

/*! \brief Set mode
 *
 * Set the graphics mode, taking into account the Windowed and Stretched
 * settings.
 */
void set_graphics_mode()
{
    const auto fsOrWindowed = isWindowed ? GFX_AUTODETECT_WINDOWED : GFX_AUTODETECT;

    if (stretch_view == 1)
    {
        set_gfx_mode(fsOrWindowed, KQ_SCALED_SCREEN_W, KQ_SCALED_SCREEN_H, 0, 0);
    }
    else
    {
        set_gfx_mode(fsOrWindowed, KQ_SCREEN_W, KQ_SCREEN_H, 0, 0);
    }

    set_palette(pal);
}

/*! \brief Show keys help
 * Show a screen with the keys listed, and other helpful info
 * \author PH
 * \date 20030527
 */
void show_help()
{
    kqDraw.menubox(double_buffer, 116 + xofs, yofs, 9, 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
    kqDraw.print_font(double_buffer, 132 + xofs, 8 + yofs, _("KQ Help"), eFontColor::FONTCOLOR_GOLD);
    kqDraw.menubox(double_buffer, 32 + xofs, 32 + yofs, 30, 20, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
    kqDraw.menubox(double_buffer, xofs, 216 + yofs, 38, 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
    kqDraw.print_font(double_buffer, 16 + xofs, 224 + yofs, _("Press CONFIRM to exit this screen"), eFontColor::FONTCOLOR_NORMAL);
    citem(72, _("Up Key:"), kq_keyname(PlayerInput.kup), eFontColor::FONTCOLOR_NORMAL);
    citem(80, _("Down Key:"), kq_keyname(PlayerInput.kdown), eFontColor::FONTCOLOR_NORMAL);
    citem(88, _("Left Key:"), kq_keyname(PlayerInput.kleft), eFontColor::FONTCOLOR_NORMAL);
    citem(96, _("Right Key:"), kq_keyname(PlayerInput.kright), eFontColor::FONTCOLOR_NORMAL);
    citem(104, _("Confirm Key:"), kq_keyname(PlayerInput.kalt), eFontColor::FONTCOLOR_NORMAL);
    citem(112, _("Cancel Key:"), kq_keyname(PlayerInput.kctrl), eFontColor::FONTCOLOR_NORMAL);
    citem(120, _("Menu Key:"), kq_keyname(PlayerInput.kenter), eFontColor::FONTCOLOR_NORMAL);
    citem(128, _("System Menu Key:"), kq_keyname(PlayerInput.kesc), eFontColor::FONTCOLOR_NORMAL);
    do
    {
        kqDraw.blit2screen(xofs, yofs);
        PlayerInput.readcontrols();
    }
    while (!PlayerInput.balt && !PlayerInput.bctrl);
    Game.unpress();
}

/*! \brief Initialize sound system
 * \author JB
 * \date ????????
 * \remark On entry is_sound=1 to initialize,
 *         on exit is_sound=0 (failure) or 2 (success),
 *         is_sound=2 to shutdown,
 *         on exit is_sound=0
 * \remark 20020914 - 05:28 RB : Updated
 *  20020922 - ML : updated to use DUMB
 *  20020922 - ML : Changed to only reserving 8 voices. (32 seemed over-kill?)
 */
void sound_init()
{
    if (!sound_avail)
    {
        is_sound = 0;
        return;
    }
    switch (is_sound)
    {
    case 1:
        /* set_volume_per_voice (2); */
        Music.init_music();
        is_sound = load_samples() ? 0 : 2; /* load the wav files */
        break;
    case 2:
        /* TT: We forgot to add this line, causing phantom music to loop */
        Music.stop_music();
        free_samples();
        is_sound = 0;
        break;
    }
}
