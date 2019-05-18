#pragma once

#define MAX_ITEMS 9

enum eItemEffectResult
{
    ITEM_EFFECT_INEFFECTIVE = 0,
    ITEM_EFFECT_SUCCESS_SINGLE = 1,
    ITEM_EFFECT_SUCCESS_MULTIPLE = 2
};

void camp_item_menu();
int check_inventory(size_t, int);
eItemEffectResult item_effects(size_t, size_t, int);
void remove_item(size_t, int);
int useup_item(int);
