#pragma once

/*! \file */

/*  RB IDEA: We can use the COUNT definition of the new datafile  */
/*           dump rather than hardcoding the value here.          */
#define MAX_SAMPLES 43

#define SND_MENU 0
#define SND_CLICK 1
#define SND_BAD 2
#define SND_ITEM 3
#define SND_EQUIP 4
#define SND_UNEQUIP 5
#define SND_MONEY 6
#define SND_TWINKLE 7
#define SND_EXPLODE 42

/*  This is in addition to setup.c:  */
void parse_setup();
void config_menu();
void show_help();
void set_graphics_mode();
void sound_init();
void free_samples();
void play_effect(int, int);

extern char debugging;
extern char slow_computer;

/* Allegro 4.2 has the scancode_to_name function,
 * For previous versions, we must emulate that function
 * with our own table of keynames
 */
#if (ALLEGRO_VERSION >= 4 && ALLEGRO_SUB_VERSION >= 2)
#define kq_keyname scancode_to_name
#else

/*! Look up table of names for keys */
static char* keynames[] =
{
    "",           "A",          "B",        "C",         "D",
    "E",          "F",          "G",        "H",         "I",
    "J",          "K",          "L",        "M",         "N",
    "O",          "P",          "Q",        "R",         "S",
    "T",          "U",          "V",        "W",         "X",
    "Y",          "Z",          "0",        "1",         "2",
    "3",          "4",          "5",        "6",         "7",
    "8",          "9",          "0_PAD",    "1_PAD",     "2_PAD",
    "3_PAD",      "4_PAD",      "5_PAD",    "6_PAD",     "7_PAD",
    "8_PAD",      "9_PAD",      "F1",       "F2",        "F3",
    "F4",         "F5",         "F6",       "F7",        "F8",
    "F9",         "F10",        "F11",      "F12",       "ESC",
    "TILDE",      "MINUS",      "EQUALS",   "BACKSPACE", "TAB",
    "OPENBRACE",  "CLOSEBRACE", "ENTER",    "COLON",     "QUOTE",
    "BACKSLASH",  "BACKSLASH2", "COMMA",    "STOP",      "SLASH",
    "SPACE",      "INSERT",     "DEL",      "HOME",      "END",
    "PGUP",       "PGDN",       "LEFT",     "RIGHT",     "UP",
    "DOWN",       "SLASH_PAD",  "ASTERISK", "MINUS_PAD", "PLUS_PAD",
    "DEL_PAD",    "ENTER_PAD",  "PRTSCR",   "PAUSE",     "ABNT_C1",
    "YEN",        "KANA",       "CONVERT",  "NOCONVERT", "AT",
    "CIRCUMFLEX", "COLON2",     "KANJI",    "LSHIFT",    "RSHIFT",
    "LCONTROL",   "RCONTROL",   "ALT",      "ALTGR",     "LWIN",
    "RWIN",       "MENU",       "SCRLOCK",  "NUMLOCK",   "CAPSLOCK"
};

#define N_KEYNAMES (sizeof(keynames) / sizeof(*keynames))
const char* kq_keyname(int);
#endif
