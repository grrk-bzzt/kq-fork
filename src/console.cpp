#include "console.h"
#include "constants.h"
#include "draw.h"
#include "gfx.h"
#include "input.h"
#include "kq.h"
#include "music.h"
#include "structs.h"
#include <stddef.h>

/*! \file
* \brief Lua console for debugging
* \author PH
* \date 20070723
*/

/* Internal variables */
static struct console_state
{
#define CONSOLE_LINES 25
    char* lines[CONSOLE_LINES];
    char inputline[80];
    int cursor;
    int on;
} kqConsole;

enum eRunConsoleKeys
{
    RUNKEY_RETURN = '\r',
    RUNKEY_DELETE = 127,
    RUNKEY_CTRL_G = 7,
    RUNKEY_BACKSPACE = 8,
    RUNKEY_CTRL_R = 18,
    RUNKEY_CTRL_S = 19
};

/*! \brief Initialize the console state
*
* Set up the global state ready for using the console
* \author PH
*/
void init_console()
{
    size_t console_line;

    kqConsole.cursor = 0;
    kqConsole.on = 0;
    for (console_line = 0; console_line < CONSOLE_LINES; ++console_line)
    {
        kqConsole.lines[console_line] = nullptr;
    }
}

/*! \brief Show the current console
*
* Display the current state of the console on the double
* buffer. This includes a horizontal line. The console takes
* up 320x120 pixels.
* \author PH
* \param xofs x-offset display position
* \param yofs y-offset display position
*/
void display_console(uint32_t xofs, uint32_t yofs)
{
    uint32_t i, y;
    uint32_t max_y = yofs + 120;

    if (kqConsole.on != 1)
    {
        return;
    }
    rectfill(double_buffer, xofs, max_y, xofs + 320, yofs + 240, makecol(0, 0, 0));
    hline(double_buffer, xofs, max_y, xofs + 320, makecol(255, 255, 255));
    y = yofs + 240 - 2 * text_height(font);
    i = CONSOLE_LINES - 1;
    while (y > max_y)
    {
        if (kqConsole.lines[i])
        {
            kqDraw.print_font(double_buffer, xofs, y, kqConsole.lines[i], eFontColor::FONTCOLOR_GREEN);
        }
        y -= text_height(font);
        --i;
    }
    kqDraw.print_font(double_buffer, xofs, yofs + 240 - 8, kqConsole.inputline, eFontColor::FONTCOLOR_NORMAL);
    rectfill(double_buffer, xofs + text_length(font, kqConsole.inputline), yofs + 238, xofs + text_length(font, kqConsole.inputline) + text_length(font, "_"), yofs + 240, makecol(192, 192, 192));
}

/*! \brief Display a line on the console
*
* This displays a line of text, scrolling up all the other
* lines. No wrapping is performed.
* \param l the text to display
*/
void scroll_console(const char* l)
{
    int i;

    if (l == nullptr)
    {
        return;
    }
    free(kqConsole.lines[0]);
    for (i = 0; i < CONSOLE_LINES - 1; ++i)
    {
        kqConsole.lines[i] = kqConsole.lines[i + 1];
    }
    kqConsole.lines[CONSOLE_LINES - 1] = strcpy((char*)malloc(strlen(l) + 1), l);
}

/* \brief Enter console mode
*
* Run the console. Does not return until the console
* is closed.
*/
void run_console()
{
    int c;
    size_t sl;
    int running;
    uint32_t string_len;
    uint32_t i;
    static const char get[] = "return get_progress(P_";
    static const char ret[] = "return ";
    static const char set[] = "set_progress(P_";

    kqConsole.inputline[0] = '\0';
    kqConsole.on = 1;

    /* Wait for all keys up */
    while (keypressed())
    {
        readkey();
    }
    running = 1;
    while (running == 1)
    {
        sl = strlen(kqConsole.inputline);

        /* Get a key */
        while (!keypressed())
        {
            Game.do_check_animation();
            kqDraw.blit2screen(xofs, yofs);
            Music.poll_music();
            Game.kq_yield();
        }

        switch ((c = readkey()) & 0xff)
        {
        case RUNKEY_RETURN: /* Return */
            if (sl == 0)
            {
                /* Stop when blank line is entered */
                running = 0;
                kqConsole.on = 0;
            }
            else
            {
                kqConsole.on = 0;
                scroll_console(kqConsole.inputline);
                do_console_command(kqConsole.inputline);
                kqConsole.inputline[0] = '\0';
                kqConsole.on = 1;
            }
            break;

        case RUNKEY_DELETE: /* delete */
            if (strlen(kqConsole.inputline) > 0)
            {
                kqConsole.inputline[sl - 1] = '\0';
            }
            break;

        case RUNKEY_CTRL_G: /* ctrl g */
            do_console_command(kqConsole.inputline);

            string_len = strlen(get);
            for (i = 0; i < string_len; i++)
            {
                kqConsole.inputline[i] = get[i];
            }
            break;

        case RUNKEY_BACKSPACE: /* backspace */
            if (strlen(kqConsole.inputline) > 0)
            {
                kqConsole.inputline[sl - 1] = '\0';
            }
            break;

        case RUNKEY_CTRL_R: /* ctrl r */
            do_console_command(kqConsole.inputline);

            string_len = strlen(ret);
            for (i = 0; i < string_len; i++)
            {
                kqConsole.inputline[i] = ret[i];
            }
            break;

        case RUNKEY_CTRL_S: /* ctrl s */
            do_console_command(kqConsole.inputline);

            string_len = strlen(set);
            for (i = 0; i < string_len; i++)
            {
                kqConsole.inputline[i] = set[i];
            }
            break;

        default:
            if (strlen(kqConsole.inputline) < sizeof(kqConsole.inputline) - 1)
            {
                kqConsole.inputline[sl] = c & 0xff;
                kqConsole.inputline[sl + 1] = '\0';
            }
            break;
        }
    }

    /* Wait for enter key up */
    do
    {
        PlayerInput.readcontrols();
    }
    while (PlayerInput.benter);
}
