#pragma once

#include "kq.h"

void add_questinfo(const char* key, const char* text);
void do_questinfo();
void draw_mainmenu(int);
void draw_playerstat(Raster*, int, int, int);
int give_xp(int, int, int);
void menu();
void revert_equipstats();
void spec_items();
void update_equipstats();

bool player2fighter(size_t partyIndex, KFighter& pf);

/* These are hints/reminders about the game - e.g:
 * - Something you have been told by an important character
 * - What you should do next
 * - Some info about a hero
 *
 * ... anything, really!
 *
 * \author PH
 * \date 20050429
 */
struct info_item
{
    char* key;  /*!< The identifying title */
    char* text; /*!< The actual info */
};

/* Dynamic array for the hints/reminders */
struct info_list
{
    info_item* root;  /*!< The array of active info items */
    int count;    /*!< The number of items currently in the array */
    int capacity; /*!< The total capacity of the array */
};
