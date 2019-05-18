#pragma once

/*! \file
 * \brief Main include file for KQ
 * \author JB
 * \date ??????
 */

/* Have to undef some stuff because Allegro defines it - thanks guys
*/
#ifdef HAVE_CONFIG_H
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_BUGREPORT
#include "config.h"
#endif

#ifdef __GNUC__
#define NORETURN __attribute__((noreturn))
#else
#ifdef _MSC_VER
#define NORETURN __declspec(noreturn)
#else
#define NORETURN
#endif /* MSVC */
#endif /* GNUC */

#include <stdint.h>
#include <string>

#include "gettext.h"
#define _(s) gettext(s)

#include "constants.h"
#include "enums.h"
#include "fighter.h"
#include "heroc.h"
#include "maps.h"
#include "player.h"
#include "structs.h"

class Raster;

class KGame
{
public:
    KGame();

    /**
     * Free old map data and load a new one
     *
     * This loads a new map and performs all of the functions
     * that accompany the loading of a new map.
     *
     * \param   map_name Base name of map (xxx -> maps/xxx.map)
     * \param   msx New x-coord for player. Pass 0 for msx and msy
     *              to use the 'default' position stored in the
     *              map file: s_map::stx and s_map::sty
     * \param   msy New y-coord for player
     * \param   mvx New x-coord for camera. Pass 0 for mvx and mvy
     *              to use the default: s_map::stx and s_map::sty)
     * \param   mvy New y-coord for camera
     */
    void change_map(const std::string& map_name, int msx, int msy, int mvx, int mvy);

    /**
     * Free old map data and load a new one.
     *
     * Load the specified map and locate the coordinates where to place the player based
     * on a marker name.
     *
     * \param   map_name Base name of map (xxx -> maps/xxx.map)
     * \param   marker_name Marker within the map file containing both x and y coords. If
     *          the marker's name doesn't exist on the map, pass 0 for msx and msy to use
     *          the 'default' position stored in the map file (s_map::stx and s_map::sty)
     * \param   offset_x Start player this many tiles left/right from the marker
     * \param   offset_y Start player this many tiles up/down from the marker
     */
    void change_map(const std::string& map_name, const std::string& marker_name, int offset_x, int offset_y);

    /**
     * Calculate the viewport so the player is displayed within it.
     *
     * This is used to determine what part of the map is
     * visible on the screen.  Usually, the party can walk around
     * in the center of the screen a bit without causing it to
     * scroll.  The center parameter is mostly used for warps and
     * such, so that the players start in the center of the screen.
     *
     * \param   center Unused variable
     */
    void calc_viewport();

    /**
     * Zone event handler
     *
     * This routine is called after every final step onto
     * a new tile (not after warps or such things).  It
     * just checks if the zone value for this coordinate is
     * not zero and then it calls the event handler.  However,
     * there is a member of the map structure called zero_zone
     * that let's you call the event handler on 0 zones if you
     * wish.
     * This function also handles the Repulse functionality
     */
    void zone_check() const;

    /**
     * Move player(s) to new coordinates
     *
     * Fade out... change coordinates... fade in.
     * The wtx/wty coordinates indicate where to put the player.
     * The wvx/wvy coordinates indicate where to put the camera.
     *
     * \param   wtx New x-coord
     * \param   wty New y-coord
     * \param   fspeed Speed of fading (See KDraw::do_transition())
     */
    void warp(int wtx, int wty, int fspeed);

    /**
     * Do tile animation
     *
     * This updates tile indexes for animation threads.
     * Animations within tilemaps consist of a starting tile index, an ending
     * tile index, and a delay. The smaller the delay value, the faster that the
     * animation cycles through the tiles.
     */
    void do_check_animation();

    /**
     * Alt key handler
     *
     * This function is called when the player presses the 'alt' key.
     * Things that can be activated are entities and zones that are
     * obstructed.
     */
    void activate();

    /**
     * Wait for key release
     *
     * This is used to wait and make sure that the user has
     * released a key before moving on.
     * 20030728 PH re-implemented in IMHO a neater way
     *
     * \note Waits at most 20 'ticks'
     */
    void unpress();

    /**
     * Wait for ALT
     *
     * Simply wait for the 'alt' key to be pressed.
     */
    void wait_enter();

    /**
     * Log events
     *
     * This is for logging events within the program.  Very
     * useful for debugging and tracing.
     * \note klog is deprecated; use Allegro's TRACE instead.
     *
     * \param   msg String to add to log file
     */
    void klog(const char* msg);

    /**
     * Pause for a time
     *
     * Why not just use rest() you ask?  Well, this function
     * kills time, but it also processes entities.  This function
     * is basically used to run entity scripts and for automatic
     * party movement.
     *
     * \param   dtime Time in frames
     */
    void kwait(int dtime);

    /**
     * End program due to fatal error
     *
     * Kill the program and spit out a message.
     *
     * \param   message Text to put into log
     */
    NORETURN void program_death(const char* message);

    /**
     * Is this character in the party?
     *
     * Determine whether the specified character is currently in play.
     *
     * \param   pn Character to ask about
     * \returns index of member's ID if found, else ePIDX::MAXCHRS if NOT in party.
     */
    size_t in_party(ePIDX);

    /**
     * Wait for scripted movement to finish
     *
     * This does like kq_wait() and processes entities...
     * however, this function waits for particular entities
     * to finish scripted movement rather than waiting for
     * a specific amount of time to pass.
     * Specify a range of entities to wait for.
     * \note 20030810 PH implemented this in a neater way, need to check if it
     * always works though.
     *
     * \param   first_entity_index First entity
     * \param   last_entity_index Last entity
     */
    void wait_for_entity(size_t first_entity_index, size_t last_entity_index);

    /* \brief Get the next event if any
     *
     * Checks the pending events and returns the name
     * of the next one, or NULL if nothing is ready
     * to be triggered.
     * If more than one event is ready, only one will be returned;
     * the next one will be returned next time.
     * Each event is removed after it is triggered. If a repeating
     * event is desired, you should call add_timer_event() again
     * in the handler function.
     *
     * \returns name of the next event or NULL if none is ready
     */
    char* get_timer_event();

    /* \brief Add a new timer event to the list
     *
     * Up to five pending events can be stored
     * at once.
     * \param n the name of the event
     * \param delta the number of seconds before the
     *        event will be called. For example 5 means
     *        five seconds in the future
     * \returns <0 if an error occurred (i.e. too many pending events)
     */
    int add_timer_event(const char*, int);

    /**
     * Delete any pending events
     *
     * This removes any events from the list
     */
    void reset_timer_events();

    /**
     * Resets the world. Called every new game and load game
     *
     * This function may be called multiple times in some cases. That should be ok.
     */
    void reset_world();

    /**
     * Yield processor for other tasks
     *
     * This function calls rest() with the value of 'cpu_usage' as its parameter
     */
    void kq_yield();

    /**
     * Creates a bitmap, giving an error message with the specified name if it fails.
     *
     * This function terminates the program with an error message if it fails to
     * allocate the specified bitmap. The name supplied is shown if this happens
     * to help you trace which bitmap caused the issue.
     *
     * \param   bitmap_width Width
     * \param   bitmap_height Height
     * \param   bitmap_name Name of bitmap
     * \returns the pointer to the created bitmap
     */
    Raster* alloc_bmp(int bitmap_width, int bitmap_height, const char* bitmap_name);

    /**
     * Application start-up code
     *
     * Set up allegro, set up variables, load stuff, blah...
     * This is called once per game.
     */
    void startup();

    /**
     * Free allocated memory
     *
     * This frees memory and such things.
     */
    void deallocate_stuff();

    /**
     * Create bitmaps
     *
     * A separate function to create all global bitmaps needed in the game.
     */
    void allocate_stuff();

    /**
     * Load initial hero stuff from file
     *
     * Loads the hero stats from a file.
     *
     */
    void load_heroes();

    /**
     * Do everything necessary to load a map
     *
     * \param   msx - New x-coord for player
     * \param   msy - Same, for y-coord
     * \param   mvx - New x-coord for camera
     * \param   mvy - Same, for y-coord
     */
    void prepare_map(int msx, int msy, int mvx, int mvy);

    /**
     * Write debug data to disk
     *
     * Writes the treasure and progress arrays in text format to "treasure.log"
     * and "progress.log" respectively. This happens in response to user hitting
     * the F11 key.
     */
    void data_dump();

    std::string GetCurmap();

    void SetCurmap(const std::string& curmap);

    bool IsOverworldMap() const;

public:
    const std::string WORLD_MAP;

    /*! The number of frames per second */
    const int32_t KQ_TICKS;

protected:
    /*! Name of the current map */
    std::string m_curmap;
};

extern int camera_viewport_x, camera_viewport_y, mx, my, steps, lastm[MAX_PARTY_SIZE];

extern Raster* double_buffer, *fx_buffer;

/**
 * Each map specifies the name of its .png file within the maps/*.tsx files.
 * Each tile is stored in an individual Raster.
 */
extern Raster* map_icons[MAX_TILES];

/**
 * Used for animations within tilesets (animation tiles must be sequential).
 *
 * The current tile to be drawn to double_buffer is defined as map_icons[tilex[???]].
 * tilex[] is simply a 0-based value ranging from 0..MAX_TILES.
 *
 * Another array, such as map_seg[idx], b_seg[idx], f_seg[idx], etc., populates '???'
 * and may update the tile with value 'idx' to the next value in the animation set.
 */
extern uint16_t tilex[MAX_TILES];

extern Raster* back, *tc, *tc2, *bub[8], *b_shield, *b_shell, *b_repulse, *b_mp;
extern Raster* cframes[NUM_FIGHTERS][MAXCFRAMES], *tcframes[NUM_FIGHTERS][MAXCFRAMES], *frames[ePIDX::MAXCHRS][MAX_PARTY_MOVEMENT_FRAMES];
extern Raster* eframes[TOTAL_MAP_NPC_ENTITIES][MAX_NPC_MOVEMENT_FRAMES], *pgb[9], *sfonts[5];
extern Raster* bord[8];
extern Raster* menuptr, *mptr, *sptr, *stspics, *sicons, *bptr, *missbmp, *noway, *upptr, *dnptr;
extern Raster* shadow[MAX_SHADOWS];
extern unsigned short* map_seg;
extern unsigned short* b_seg, *f_seg;
extern unsigned char* z_seg, *s_seg, *o_seg;
extern unsigned char progress[SIZE_PROGRESS];
extern unsigned char treasure[SIZE_TREASURE];
extern unsigned char save_spells[SIZE_SAVE_SPELL];
extern Raster* kfonts;
extern s_map g_map;
extern KQEntity allEntitiesOnTheMap[MAX_ENTITIES];
extern s_anim tanim[MAX_TILESETS][MAX_ANIM];
extern s_anim adata[MAX_ANIM];
extern uint32_t numchrs;
extern int gp, xofs, yofs, gsvol, gmvol;
extern ePIDX activeAvatarIds[ePIDX::MAXCHRS];
extern uint8_t autoparty, alldead, is_sound, deadeffect, vfollow, use_sstone, sound_avail;
extern const uint8_t kq_version;
extern uint8_t hold_fade, cansave, skip_intro, wait_retrace, stretch_view, cpu_usage;
extern uint16_t adelay[MAX_ANIM];
extern char* strbuf, *savedir;
extern KPlayer party[ePIDX::MAXCHRS];
extern s_heroinfo players[ePIDX::MAXCHRS];
extern bool display_attack_string;
extern std::string shop_name;
extern char attack_string[39];
extern volatile int timer, ksec, kmin, khr, animation_count, timer_count;
extern COLOR_MAP cmap;
extern uint8_t can_run, display_desc;
extern uint8_t draw_background, draw_middle, draw_foreground, draw_shadow;
extern s_inventory g_inv[MAX_INV];
extern s_special_item special_items[MAX_SPECIAL_ITEMS];
extern short player_special_items[MAX_SPECIAL_ITEMS];
extern int view_x1, view_y1, view_x2, view_y2, view_on, in_combat;
extern int show_frate, use_joy;

/*! Variables used with KQ_CHEATS */
extern bool hasCheatEnabled;
extern int no_random_encounters;
extern int every_hit_999;
extern int no_monsters;

#ifdef DEBUGMODE
extern Raster* obj_mesh;
#endif

extern KGame Game;

#ifndef TRACE
extern void TRACE(const char* message, ...);
#endif
