/*! \file
 * \brief Stuff relating to credits display:
 *        The original authors, new team etc are name-checked
 * \author PH
 * \date 20030526
 */

#include "credits.h"
#include "constants.h"
#include "draw.h"
#include "gettext.h"
#include "gfx.h"
#include <string>

#define _(s) gettext(s)

static int ease(int);

/*! Array of strings */
static const char* credits[] = {
    "(C) 2001 DoubleEdge Software",
    "(C) 2002-2016 KQ Lives Team",
    "http://kqlives.sourceforge.net/",
    "Peter Hull",
    "TeamTerradactyl",
    "Chris Barry",
    "Eduardo Dudaskank",
    "Troy D Patterson",
    "Master Goodbytes",
    "Rey Brujo",
    "Matthew Leverton",
    "Sam Hocevar",
    "Günther Brammer",
    "WinterKnight",
    "Edgar Alberto Molina",
    "Steven Fullmer (OnlineCop)",
    nullptr
};

static const int NUM_EASE_VALUES = 32;

static const char** cc = nullptr;
static short int ease_table[NUM_EASE_VALUES];
static Raster* wk = nullptr;

static volatile uint32_t ticks = UINT32_MAX;

void allocate_credits()
{
    if (wk == nullptr)
    {
        unsigned int tlen = 0;

        // Determine the longest text in the credits.
        for (auto credits_current_line = credits; *credits_current_line; ++credits_current_line)
        {
            size_t current_line_length = strlen(*credits_current_line);
            if (current_line_length > tlen)
            {
                tlen = current_line_length;
            }
        }
        wk = new Raster(8 * tlen, NUM_EASE_VALUES * 2);

        // Pre-generate the ease_table values, so they don't have
        // to be calculated on the fly at runtime. All calculations
        // are integer division.
        for (int ease_index = 0; ease_index < NUM_EASE_VALUES; ++ease_index)
        {
            ease_table[ease_index] = short(ease_index * ease_index * (3 * NUM_EASE_VALUES - 2 * ease_index) / NUM_EASE_VALUES / NUM_EASE_VALUES);
        }
    }
    cc = credits;
}

void deallocate_credits()
{
    delete (wk);
    wk = nullptr;
}

void display_credits(Raster* double_buffer)
{
    static const uint32_t max_ticks = 640;

    static const char* pressf1 = _("Press F1 for help");
    if (ticks > max_ticks)
    {
        clear_bitmap(wk);
        kqDraw.print_font(wk, (wk->width - 8 * strlen(*cc)) / 2, 42, *cc, eFontColor::FONTCOLOR_NORMAL);

        /* After each 'max_ticks' number of ticks, increment the current line of
         * credits displayed, looping back to the beginning as needed.
         */
        if (*(++cc) == nullptr)
        {
            cc = credits;
        }
        kqDraw.print_font(wk, (wk->width - 8 * strlen(*cc)) / 2, 10, *cc, eFontColor::FONTCOLOR_NORMAL);
        ticks = 0;
    }
    else
    {
        ++ticks;
    }

    int ease_amount = (max_ticks / 2) - ticks;
    int x0 = (320 - wk->width) / 2;
    for (int i = 0; i < wk->width; ++i)
    {
        blit(wk, double_buffer, i, ease(i + ease_amount), i + x0, KQ_SCREEN_H - 55, 1, 32);
    }
    kqDraw.print_font(double_buffer, (KQ_SCREEN_W - 8 * strlen(pressf1)) / 2, KQ_SCREEN_H - 30, pressf1, eFontColor::FONTCOLOR_NORMAL);
#ifdef KQ_CHEATS
    /* Put an un-ignorable cheat message; this should stop
     * PH releasing versions with cheat mode compiled in ;)
     */
    extern bool hasCheatEnabled;
    kqDraw.print_font(double_buffer, 80, 40, hasCheatEnabled ? _("*CHEAT MODE ON*") : _("*CHEAT MODE OFF*"), eFontColor::FONTCOLOR_GOLD);
#endif
#ifdef DEBUGMODE
    /* TT: Similarly, if we are in debug mode, we should be warned. */
    kqDraw.print_font(double_buffer, 80, 48, _("*DEBUG MODE ON*"), eFontColor::FONTCOLOR_GOLD);
#endif
}

/*! \brief An S-shaped curve
 *
 * Returns values from an 'ease' curve, generally 3*x^2 - 2*x^3,
 * but clamped to 0..NUM_EASE_VALUES (inclusive).
 *
 * \param   x Where to evaluate the function
 * \returns Clamped integer value between 0 and NUM_EASE_VALUES, inclusive.
 */
static int ease(int x)
{
    if (x <= 0)
    {
        return 0;
    }
    else if (x >= NUM_EASE_VALUES)
    {
        return NUM_EASE_VALUES;
    }
    else
    {
        return ease_table[x];
    }
}
