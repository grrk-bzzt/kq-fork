#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <allegro.h>
#include "kq.h"
#include "disk.h"

enum plr_props
{
   ID_ERR = -1,
   ID_NAME = 0,
   ID_MHP,
   ID_MMP,
   ID_MRP,
   ID_LVL,
   ID_NEXT,
   ID_XP,
   ID_ANIM,
   ID_A = 100,
   ID_R = 200,
};

/* JB's defaults */


/* *INDENT-OFF* */
s_player default_players[ePIDX::MAXCHRS] = {
   {"Sensar", 0, 70, 1, 100, 40, 40, 0, 0,
    {800, 500, 700, 300, 300, 4000, 9000, 9000, 0, 7500, 0, 100, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
   {"Sarina", 0, 70, 1, 100, 36, 36, 4, 4,
    {600, 600, 600, 400, 400, 6000, 9000, 9000, 0, 6000, 0, 100, 0},
    {0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
   {"Corin", 0, 70, 1, 100, 25, 25, 15, 15,
    {400, 400, 500, 800, 400, 4500, 12000, 9000, 0, 5000, 0, 100, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
   {"Ajathar", 0, 70, 1, 100, 30, 30, 8, 8,
    {600, 500, 500, 400, 800, 5000, 9000, 11000, 0, 5000, 0, 100, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
   {"Casandra", 0, 70, 1, 100, 24, 24, 16, 16,
    {300, 600, 400, 800, 600, 5500, 10000, 10000, 0, 5000, 0, 100, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 8, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
   {"Temmin", 0, 70, 1, 100, 35, 35, 5, 5,
    {700, 500, 800, 300, 500, 5500, 8000, 10000, 0, 6000, 0, 100, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
   {"Ayla", 0, 70, 1, 100, 32, 32, 8, 8,
    {500, 800, 500, 600, 400, 7000, 10000, 8000, 0, 5000, 0, 100, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
   {"Noslom", 0, 70, 1, 65, 22, 22, 18, 18,
    {300, 600, 300, 700, 700, 5500, 10000, 10000, 0, 5000, 0, 100, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}
};


s_tileset default_tilesets[MAX_TILESETS] = {
   {"LAND_PCX", {{2, 5, 25}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}},
   {"NEWTOWN_PCX", {{158, 159, 50}, {160, 163, 25}, {176, 179, 25}, {257, 258, 50}, {262, 263, 25}}},
   {"CASTLE_PCX", {{57, 58, 50}, {62, 63, 25}, {205, 206, 50}, {250, 253, 25}, {0, 0, 0}}},
   {"INCAVE_PCX", {{30, 35, 30}, {176, 179, 25}, {323, 328, 40}, {380, 385, 40}, {0, 0, 0}}},
   {"VILLAGE_PCX", {{38, 39, 25}, {80, 83, 25}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}},
   {"MOUNT_PCX", {{58, 59, 50}, {40, 42, 50}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}},
   {"SHRINE_PCX", {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}},
   {"FORTRESS_PCX", {{120, 123, 30}, {124, 129, 25}, {130, 133, 30}, {134, 137, 25}, {138, 139, 30}}},
};

/* *INDENT-ON* */
static int get_knum (const char *);
static char *split (char *);
static int save_s_player_txt (s_player *, FILE *);
static const char *get_kname (int);


/*! \brief Process a player character file
 *
 * \param in file with ascii data
 * \param out output file
 *
 * \returns 0 if OK, otherwise an error
 */
int process_plr (FILE *in, PACKFILE *out)
{
   char buffer[128], *val;
   s_player plr;
   int lines;
   int pl_new = 1;
   int key, nk;
   while (fgets (buffer, sizeof (buffer), in) != NULL) {
      if (pl_new) {
         memset (&plr, 0, sizeof (plr));
         pl_new = 0;
         lines = 0;
      }

      if (buffer[0] != '#' && (val = split (buffer)) != NULL) {
         ++lines;
         key = get_knum (buffer);
         switch (key) {
         case ID_LVL:
            plr.lvl = atoi (val);
            break;
         case ID_NEXT:
            plr.next = atoi (val);
            break;
         case ID_NAME:
            strncpy (plr.name, val, sizeof (plr.name));
            break;
         case ID_MHP:
            plr.mhp = plr.hp = atoi (val);
            break;
         case ID_MMP:
            plr.mmp = plr.mp = atoi (val);
            break;
         case ID_MRP:
            plr.mrp = atoi (val);
         case ID_XP:
            plr.xp = atoi (val);
            break;
         default:
            nk = key - ID_A;
            if (nk >= 0 && nk < NUM_STATS) {
               plr.stats[nk] = atoi (val);
            } else {
               nk = key - ID_R;
               if (nk >= 0 && nk < R_TOTAL_RES) {
                  plr.res[nk] = atoi (val);
               }
            }
            break;
         case ID_ERR:
            fprintf (stderr, "Unknown key: %s\n", buffer);

            break;
         }
      } else if (lines > 0) {

         pl_new = 1;
         save_s_player (&plr, out);
      }
   }

   if (!pl_new) {
      save_s_player (&plr, out);
   }
   return 0;
}


int save_s_player_txt (s_player * plr, FILE * out)
{
   size_t i;
   fprintf (out, "name: %s\n", plr->name);
   fprintf (out, "level: %d\n", plr->lvl);
   fprintf (out, "next: %d\n", plr->next);
   fprintf (out, "mhp: %d\n", plr->mhp);
   fprintf (out, "mmp: %d\n", plr->mmp);
   fprintf (out, "mrp: %d\n", plr->mrp);
   fprintf (out, "xp: %d\n", plr->xp);
   for (i = 0; i < NUM_STATS; ++i)
      if (plr->stats[i])
         fprintf (out, "%s: %d\n", get_kname (i + ID_A), plr->stats[i]);
   for (i = 0; i < R_TOTAL_RES; ++i)
      if (plr->res[i])
         fprintf (out, "%s: %d\n", get_kname (i + ID_R), plr->res[i]);
   fprintf (out, "\n");
   return 0;
}


/*! \brief Process a tilesets file
 *
 * \param in file with ascii data
 * \param out output file
 *
 * \returns 0 if OK, otherwise an error
 */
int process_ts (FILE *in, PACKFILE *out)
{
   char buffer[128], *val;
   s_tileset ts;
   int lines, anims;
   int ts_new = 1;
   int key;
   while (fgets (buffer, sizeof (buffer), in) != NULL) {
      if (ts_new) {
         memset (&ts, 0, sizeof (ts));
         ts_new = 0;
         lines = 0;
         anims = 0;
      }
      if (buffer[0] != '#' && (val = split (buffer)) != NULL) {
         ++lines;
         key = get_knum (buffer);
         switch (key) {
         case ID_NAME:
            strncpy (ts.icon_set, val, sizeof (ts.icon_set) - 1);
            break;
         case ID_ANIM:
            if (anims < MAX_ANIM) {
               sscanf (val, "%hd %hd %hd", &ts.tanim[anims].start, &ts.tanim[anims].end, &ts.tanim[anims].delay);
            } else if (anims == MAX_ANIM) {
               fprintf (stderr, "Error; max number of animations in one tileset is %d", MAX_ANIM);
            }
            ++anims;
            break;
         }
      } else if (lines > 0) {
         ts_new = 1;
         save_s_tileset (&ts, out);
      }
   }
   if (lines > 0) {
      save_s_tileset (&ts, out);
   }
   return 0;
}


int save_s_tileset_txt (s_tileset *ts, FILE *out)
{
   int i;
   fprintf (out, "name: %s\n", ts->icon_set);
   for (i = 0; i < MAX_ANIM; ++i) {
      if (ts->tanim[i].start != ts->tanim[i].end) {
         fprintf (out, "anim: %hd %hd %hd\n", ts->tanim[i].start, ts->tanim[i].end, ts->tanim[i].delay);
      }
   }
   fprintf (out, "\n");
   return 0;
}


int main (int argc, char *argv[])
{
   FILE *in;
   int i;
   char buffer[128];
   char *outfilename;
   PACKFILE *fout;

   enum
   {
      UNKNOWN,
      PLAYER,
      MONSTER,
      TILESET
   } type;

   allegro_init ();
   if (argc == 1) {
      /* No files to process */
      fprintf (stdout, "# Default value dump by %s\nplayer\n", argv[0]);
      for (i = 0; i < ePIDX::MAXCHRS; ++i)
      {
        save_s_player_txt (&default_players[i], stdout);
      }
      fprintf (stdout, "# Default value dump by %s\ntileset\n", argv[0]);
      for (i = 0; i < MAX_TILESETS; ++i) {
         save_s_tileset_txt (&default_tilesets[i], stdout);
      }
      return 0;
   }

   while (--argc) {
      in = fopen (argv[argc], "r");
      if (in) {
         /* Decide what kind of file */
         while (1) {
            type = UNKNOWN;
            if (fgets (buffer, sizeof (buffer), in) == NULL) {
               break;
            }
            if (buffer[0] != '#' && buffer[0] != '\n') {
               if (strncmp (buffer, "player", 6) == 0) {
                  type = PLAYER;
               }
               if (strncmp (buffer, "monster", 7) == 0) {
                  type = MONSTER;
               }
               if (strncmp (buffer, "tileset", 7) == 0) {
                  type = TILESET;
               }
               break;
            }
         }
         switch (type) {
         case PLAYER:
            fout = pack_fopen ("../data/hero.kq", F_WRITE_PACKED);
            process_plr (in, fout);
            pack_fclose (fout);
            break;
         case TILESET:
            fout = pack_fopen ("../data/tileset.kq", F_WRITE_PACKED);
            process_ts (in, fout);
            pack_fclose (fout);
            break;
         default:
            outfilename = NULL;
            fprintf (stderr, "Unknown file type %s\n", buffer);
            break;
         }
         fclose (in);
      } else {
         fprintf (stderr, "Could not open '%s' for reading\n", argv[argc]);
         return 1;
      }
   }
   return 0;
} END_OF_MAIN ()


struct _map
{
   int v;
   char *n;
}


/* *INDENT-OFF* */
map[] =
{
   {ID_NAME,           "name"},
   {ID_MHP,            "mhp"},
   {ID_MMP,            "mmp"},
   {ID_LVL,            "level"},
   {ID_NEXT,           "next"},
   {ID_MRP,            "mrp"},
   {ID_XP,             "xp"},
   {ID_ANIM,           "anim"},
   {ID_A + A_STR,      "str"},
   {ID_A + A_AGI,      "agi"},
   {ID_A + A_VIT,      "vit"},
   {ID_A + A_INT,      "int"},
   {ID_A + A_SAG,      "sag"},
   {ID_A + A_SPD,      "spd"},
   {ID_A + A_AUR,      "aur"},
   {ID_A + A_SPI,      "spi"},
   {ID_A + A_ATT,      "att"},
   {ID_A + A_HIT,      "hit"},
   {ID_A + A_DEF,      "def"},
   {ID_A + A_EVD,      "evd"},
   {ID_A + A_MAG,      "mag"},
   {ID_R + R_EARTH,    "earth"},
   {ID_R + R_BLACK,    "black"},
   {ID_R + R_FIRE,     "fire"},
   {ID_R + R_THUNDER,  "thunder"},
   {ID_R + R_AIR,      "air"},
   {ID_R + R_WHITE,    "white"},
   {ID_R + R_WATER,    "water"},
   {ID_R + R_ICE,      "ice"},
   {ID_R + R_POISON,   "poison"},
   {ID_R + R_BLIND,    "blind"},
   {ID_R + R_CHARM,    "charm"},
   {ID_R + R_PARALYZE, "paralyze"},
   {ID_R + R_PETRIFY,  "petrify"},
   {ID_R + R_SILENCE,  "silence"},
   {ID_R + R_SLEEP,    "sleep"},
   {ID_R + R_TIME,     "time"},
};

/* *INDENT-ON* */

int n_map = sizeof (map) / sizeof (*map);
int need_init = 1;


static int _cmp (const void *pa, const void *pb)
{
   const struct _map *a = (const struct _map *) pa;
   const struct _map *b = (const struct _map *) pb;
   return strcmp (a->n, b->n);
}


int get_knum (const char *k)
{
   struct _map sr, *fn;
   if (need_init) {
      need_init = 0;
      qsort (map, n_map, sizeof (struct _map), _cmp);
   }
   while (*k && isspace (*k))
      ++k;
   sr.n = (char *) k;
   fn = (struct _map *) bsearch (&sr, map, n_map, sizeof (struct _map), _cmp);
   return fn ? fn->v : -1;
}


const char *get_kname (int v)
{
   int i;
   for (i = 0; i < n_map; ++i) {
      if (v == map[i].v)
         return map[i].n;
   }
   return NULL;
}


char *split (char *in)
{
   char *col = strchr (in, '\n');
   /* Remove any newline */
   if (col)
      *col = '\0';
   col = strchr (in, ':');
   if (col) {
      *col = '\0';
      while (*(++col) && isspace (*col)) {
      }
      return col;
   } else {
      return NULL;
   }
}
