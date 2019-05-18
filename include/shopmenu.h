#pragma once

#define NUMSHOPS 50
#define SHOPITEMS 12

void do_inn_effects(int);
void draw_shopgold();
void inn(const char*, uint32_t, int);
int shop(int);

struct s_shop
{
    char shopName[40];                     /* Name of this shop */
    uint16_t items[SHOPITEMS];         /* A list of items in this shop */
    uint16_t items_current[SHOPITEMS]; /* Quantity of this type of item */
    uint16_t items_max[SHOPITEMS];     /* Maximum quantity of this type of item */

    /* Amount of time, in minutes, it takes for this shop to replenish this item
     */
    unsigned short items_replenish_time[SHOPITEMS];
    unsigned short time; /* The last time (in minutes) that you visited this shop */
};

extern s_shop shops[NUMSHOPS];
extern uint16_t num_shops;
