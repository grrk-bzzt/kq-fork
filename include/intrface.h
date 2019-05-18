#pragma once

/* Special identifiers for bubble() */
enum eHero
{
    HERO1 = 200,
    HERO2 = 201
};

void do_autoexec();
void do_entity(int en_num);
void do_luacheat();
void do_luainit(const char* fname, int global);
void do_luakill();
void do_postexec();
void do_timefunc(const char* funcname);
void do_zone(int zn_num);
void lua_user_init();
