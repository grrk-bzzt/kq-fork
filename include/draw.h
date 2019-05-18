#pragma once

#include <allegro.h>
#include <allegro/internal/aintern.h>
#include <stdint.h>
#include <stdlib.h>
#include <string>

class Raster;

enum eMenuBoxColor
{
    DARKBLUE = 0,
    SEMI_TRANSPARENT_BLUE = 2,
    DARKRED = 4
};

enum eFontColor
{
    FONTCOLOR_NORMAL = 0,
    FONTCOLOR_RED = 1,
    FONTCOLOR_YELLOW = 2,
    FONTCOLOR_GREEN = 3,
    FONTCOLOR_DARK = 4,
    FONTCOLOR_GOLD = 5,
    FONTCOLOR_BIG = 6,

    NUM_FONT_COLORS // always last
};

enum eFont
{
    FONT_WHITE = 0,
    FONT_RED,
    FONT_YELLOW,
    FONT_GREEN,
    FONT_PURPLE,
    FONT_DECIDE,

    NUM_FONTS // always last
};

enum eBubbleStyle
{
    BUBBLE_TEXT = 0,
    BUBBLE_THOUGHT = 1,
    BUBBLE_MESSAGE = 2,

    NUM_BUBBLE_STYLES // always last
};

enum eTransitionFade
{
    TRANS_FADE_IN = 1,
    TRANS_FADE_OUT = 2,
    TRANS_FADE_WHITE = 3,

    NUM_TRANSITIONS
};

/* These should correspond with the stems found in MISC.
 * Bubbles are either solid (for speech) or wavy (for thoughts).
 */
enum eBubbleStemStyle
{
    STEM_UNDEFINED = -1,

    STEM_BOTTOM_RIGHT = 0,
    STEM_BOTTOM_LEFT = 1,
    STEM_TOP_RIGHT = 2,
    STEM_TOP_LEFT = 3,

    NUM_BUBBLE_STEMS // always last
};

class KDraw
{
public:
    /*! \brief Blit from double buffer to the screen
     *
     * This does the copy from the double_buffer to the screen... for the
     * longest time I called blit in every location (over 80 places) instead
     * of having a central function... am I a moron or what?
     * Handles frame-rate display, stretching and vsync waiting.
     *
     * \param   xw x-coord in double_buffer of the top-left of the screen
     * \param   yw y-coord in double_buffer of the top-left of the screen
     */
    void blit2screen(int xw, int yw);

    /*! \brief Scale colors
     *
     * This takes a bitmap and scales it to fit in the color range specified.
     * Output goes to a new bitmap.
     * This is used to make a monochrome version of a bitmap, for example to
     * display a green, poisoned character, or the red 'rage' effect for
     * Sensar. This relies on the palette having continuous lightness ranges
     * of one color (as the KQ palette does!).
     * An alternative would be to use makecol(), though this would incur
     * a speed penalty.
     * Another alternative would be to precalculate some maps for each case.
     *
     * \param   src Source bitmap
     * \param   dest Destination bitmap
     * \param   output_range_start Start of output color range
     * \param   output_range_end End of output color range
     */
    void color_scale(Raster* src, Raster* dest, int output_range_start, int output_range_end);

    /*! \brief Convert multiple frames
     *
     * This is used to color_scale one or more fighter frames.
     *
     * \param   fighter_index Character to convert
     * \param   output_range_start Start of output range
     * \param   output_range_end End of output range
     * \param   convert_heroes If ==1 then \cframe_index fighter_index<PSIZE means
     * convert all heroes, otherwise all enemies
     */
    void convert_cframes(size_t fighter_index, int output_range_start, int output_range_end, int convert_heroes);

    /*! \brief Restore colors
     *
     * Restore specified fighter frames to normal color. This is done
     * by blitting the 'master copy' from tcframes.
     *
     * \param   fighter_index Character to restore
     * \param   revert_heroes If ==1 then convert all heroes if fighter_index < PSIZE, otherwise convert all enemies
     */
    void revert_cframes(size_t fighter_index, int revert_heroes);

    /*! \brief Draw small icon
     *
     * Just a helper function... reduces the number of places that 'sicons'
     * has to be referenced.
     * Icons are 8x8 sub-bitmaps of sicons, representing items (sword, etc.)
     *
     * \param   where Bitmap to draw to
     * \param   ino Icon to draw
     * \param   icx x-coord
     * \param   icy y-coord
     */
    void draw_icon(Raster* where, int ino, int icx, int icy);

    /*! \brief Draw status icon
     *
     * Just a helper function... reduces the number of places that 'stspics'
     * has to be referenced.
     * Status icons are 8x8 sub-bitmaps of \p stspics, representing poisoned, etc.
     *
     * \param   where Bitmap to draw to
     * \param   cc Non-zero if in combat mode (draw
     *          using info  \p fighter[] rather than \p party[] )
     * \param   who Character to draw status for
     * \param   inum The maximum number of status icons to draw.
     *          \p inum ==17 when in combat, ==8 otherwise.
     * \param   icx x-coord to draw to
     * \param   icy y-coord to draw to
     */
    void draw_stsicon(Raster* where, int cc, int who, uint32_t inum, int icx, int icy);

    /*! \brief Check for forest square
     *
     * Helper function for the draw_char() routine.  Just returns whether or not
     * the tile at the specified coordinate is a forest tile.  This could be
     * a headache if the tileset changes!
     * Looks in the \p map_seg[] array
     * PH modified 20030309 added check for map (only main map has forest)
     *
     * \param   fx x-coord to check
     * \param   fy y-coord to check
     * \returns 1 if it is a forest square, 0 otherwise
     */
    int is_forestsquare(int fx, int fy);

    /*! \brief Draw the map
     *
     * Umm... yeah.
     * Draws the background, character, middle, foreground and shadow layers.
     * The order, and the parallaxing, is specified by the mode.
     * There are 6 modes, as set in the .map file
     *  - 0 Order BMCFS,
     *  - 1 Order BCMFS,
     *  - 2 Order BMCFS, Background parallax
     *  - 3 Order BCMFS, Background & middle parallax
     *  - 4 Order BMCFS, Middle & foreground parallax
     *  - 5 Order BCMFS, Foreground parallax
     *
     * In current KQ maps, only modes 0..2 are used, with the majority being 0.
     * Also handles the Repulse indicator and the map description display.
     * \bug PH: Shadows are never drawn with parallax (is this a bug?)
     */
    void drawmap();

    /*! \brief Draw menu box
     *
     * Draw a menubox.  This is kinda hacked because of translucency, but it
     * works.  I use the DARKBLUE define to draw a non-translucent box.
     *
     * \param   where Bitmap to draw to
     * \param   x x-coord
     * \param   y y-coord
     * \param   w Width
     * \param   h Height
     * \param   bgColor Color (see note above)
     */
    void menubox(Raster* where, int x, int y, int w, int h, eMenuBoxColor bgColor);

    /*! \brief Display string
     *
     * Display a string in a particular font on a bitmap at the specified coordinates.
     *
     * \param   where Bitmap to draw to
     * \param   sx x-coord
     * \param   sy y-coord
     * \param   msg String to draw
     * \param   font_index Font index (0..6)
     */
    void print_font(Raster* where, int sx, int sy, const char* msg, eFontColor font_index);

    /*! \brief Display number
     *
     * Display a number using the small font on a bitmap at the specified
     * coordinates and using the specified color.  This still expects the
     * number to be in a string... the function's real purpose is to use
     * a different font for numerical display in combat.
     *
     * \param   where Bitmap to draw to
     * \param   sx x-coord
     * \param   sy y-coord
     * \param   msg String to draw
     * \param   font_index Font index (0..4)
     */
    void print_num(Raster* where, int sx, int sy, const std::string& msg, eFont font_index);

    /*! \brief Display speech/thought bubble
     * \author PH
     * \date 20021220
     *
     * Displays text, like bubble_text, but passing the args
     * through the relay function first
     * \date updated 20030401 merged thought and speech
     * \sa bubble_text()
     * \param   fmt Format, B_TEXT or B_THOUGHT
     * \param   who Character that is speaking
     * \param   textToDisplay The text to display
     */
    void text_ex(eBubbleStyle fmt, int who, const std::string& textToDisplay);

    /*! \brief Alert player
     *
     * Draw a single-line message in the center of the screen and wait for
     * the confirm key to be pressed or for a specific amount of time.
     *
     * \param   m Message text
     * \param   icn Icon to display or 255 for none
     * \param   delay Time to wait (milliseconds?)
     * \param   x_m X-coord of top-left (like xofs)
     * \param   y_m Y-coord of top-left
     */
    void message(const char* m, int icn, int delay, int x_m, int y_m);

    /*! \brief Do user prompt
     *
     * Draw a text box and wait for a response.  It is possible to offer up to four
     * choices in a prompt box.
     *
     * \param   who Entity that is speaking
     * \param   numopt Number of choices
     * \param   bstyle Textbox style (B_TEXT or B_THOUGHT)
     * \param   sp1 Line 1 of text
     * \param   sp2 Line 2 of text
     * \param   sp3 Line 3 of text
     * \param   sp4 Line 4 of text
     * \returns index of option chosen (0..numopt-1)
     */
    int prompt(int who, int numopt, eBubbleStyle bstyle, const char* sp1, const char* sp2, const char* sp3, const char* sp4);

    /*! \brief prompt for user input
     *
     * Present the user with a prompt and a list of options to select from.
     * The prompt is shown, as per text_ex(), and the choices shown in
     * a separate window at the bottom. If the prompt is longer than one
     * box-full, it is shown box-by-box, until the last one, when the choices are
     * shown.
     * If there are more choices than will fit into the box at the bottom, arrows
     * are shown
     * to indicate more pages.
     * Press ALT to select; CTRL does nothing.
     *
     * \author PH
     * \date 20030417
     *
     * \param   who Which character is ASKING the question
     * \param   ptext The prompt test
     * \param   opt An array of options, null terminated
     * \param   n_opt The number of options
     * \return  option selected, 0= first option etc.
     */
    uint32_t prompt_ex(uint32_t who, const char* ptext, const char* opt[], uint32_t n_opt);

    /*! \brief Adjust view
     *
     * This merely sets the view variables for use in
     * other functions that rely on the view.
     * The view defines a subset of the map,
     * for example when you move to a house in a town,
     * the view contracts to display only the interior.
     *
     * \param   vw Non-zero to enable view, otherwise show the whole map
     * \param   x1 Top-left of view
     * \param   y1 Top-left of view
     * \param   x2 Bottom-right of view
     * \param   y2 Bottom-right of view
     */
    void set_view(int vw, int x1, int y1, int x2, int y2);

    /*! \brief Display speech/thought bubble with portrait
     * \author Z9484
     * \date 2008
     *
     * Displays text, like bubble_text, but passing the args
     * through the relay function first
     * \date updated 20030401 merged thought and speech
     * \sa bubble_text()
     * \param   fmt Format, B_TEXT or B_THOUGHT
     * \param   who Character that is speaking
     * \param   textToDisplay The text to display
     */
    void porttext_ex(eBubbleStyle fmt, int who, const std::string& textToDisplay);

    /*! \brief Make a copy of a bitmap
     *
     * Take a source bitmap and a target. If the target is NULL
     * or too small, re-allocate it.
     * Then blit it.
     *
     * \param   target Bitmap to copy to or NULL
     * \param   source Bitmap to copy from
     * \returns target or a new bitmap.
     */
    Raster* copy_bitmap(Raster* target, Raster* source);


    /*! \brief Perform one of a range of palette transitions
     *
     * Fade to black, white or to the game palette (pal)
     *
     * \param   transitionFade Any of TRANS_FADE_IN, TRANS_FADE_OUT, TRANS_FADE_WHITE
     * \param   transitionSpeed Speed of transition
     */
    void do_transition(eTransitionFade transitionFade, int transitionSpeed);

private:

    /*! \brief Get glyph index
     *
     * Convert a unicode char to a glyph index.
     * \param cp unicode character
     * \return glyph index
     * \author PH
     * \date 20071116
     * \note uses inefficient linear search for now.
     */
    int get_glyph_index(uint32_t cp);

    /*! \brief Draw border box
     *
     * Draw the fancy-pants border that I use.  I hard-draw the border instead
     * of using bitmaps, because that's just the way I am.  It doesn't degrade
     * performance, so who cares :)
     * Border is about 4 pixels thick, fitting inside (x,y)-(x2,y2)
     *
     * \param   where Bitmap to draw to
     * \param   left Top-left x-coord
     * \param   top Top-left y-coord
     * \param   right Bottom-right x-coord
     * \param   bottom Bottom-right y-coord
     */
    void border(Raster* where, int left, int top, int right, int bottom);

    /*! \brief Draw background
     *
     * Draw the background layer.  Accounts for parallaxing.
     * Parallax is on for modes 2 & 3
     */
    void draw_backlayer();

    /*! \brief Draw  box, with different backgrounds and borders
     *
     * Draw the box as described. This was suggested by CB as
     * a better alternative to the old create bitmap/blit trans/destroy bitmap
     * method.
     *
     * \author PH
     * \date 20030616
     *
     * \param   where Bitmap to draw to
     * \param   x1 x-coord of top left
     * \param   y1 y-coord of top left
     * \param   x2 x-coord of bottom right
     * \param   y2 y-coord of bottom right
     * \param   bg Color/style of background
     * \param   bstyle Style of border
     */
    void draw_kq_box(Raster* where, int x1, int y1, int x2, int y2, eMenuBoxColor bgColor, eBubbleStyle bstyle);

    /*! \brief Draw heroes on map
     *
     * Draw the heroes on the map.  It's kind of clunky, but this is also where
     * it takes care of walking in forests and only showing a disembodied head.
     * Does not seem to do any parallaxing. (?)
     * PH modified 20030309 Simplified this a bit, removed one blit() that wasn't
     * needed.
     */
    void draw_char();

    void render_npc(size_t fighter_index, int32_t dx, int32_t dy, size_t fighter_frame);

    void render_hero(size_t fighter_index, size_t fighter_frame);

    /*! \brief Draw foreground
     *
     * Draw the foreground layer.  Accounts for parallaxing.
     * Parallax is on for modes 4 & 5.
     */
    void draw_forelayer();

    /*! \brief Draw middle layer
     *
     * Draw the middle layer.  Accounts for parallaxing.
     * Parallax is on for modes 3 & 4
     */
    void draw_midlayer();

    /* Check whether the player is standing inside a bounding area. If so,
     * update the view_area coordinates before drawing to the map.
     *
     * \param   map - The map containing the bounded area data
     */
    void draw_playerbound();

    /*! \brief Draw shadows
     *
     * Draw the shadow layer... this beats making extra tiles.  This may be
     * moved in the future to fall between the background and foreground layers.
     * Shadows are never parallaxed.
     */
    void draw_shadows();

    /*! \brief Draw text box
     *
     * Hmm... I think this function draws the textbox :p
     *
     * \date 20030417 PH This now draws the text as well as just the box
     * \param   bstyle Style (B_TEXT or B_THOUGHT or B_MESSAGE)
     */
    void draw_textbox(eBubbleStyle bstyle);

    /*! \brief Draw text box with portrait
     *
     *  Shows the player's portrait and name with the text.
     *
     * \date 20081218 Z9484
     * \param   bstyle Style (B_TEXT or B_THOUGHT or B_MESSAGE)
     * \param   chr (what chr is talking)
     */
    void draw_porttextbox(eBubbleStyle bstyle, int chr);

    /*! \brief Text box drawing
     *
     * Generic routine to actually display a text box and wait for a keypress.
     *
     * \param   who Character that is speaking/thinking (ignored for B_MESSAGE
     * style)
     * \param   box_style Style (B_TEXT or B_THOUGHT or B_MESSAGE)
     */
    void generic_text(int who, eBubbleStyle box_style, int isPort);

    /*! \brief Decode String
     *
     * Extract the next unicode char from a UTF-8 string
     *
     * \param string Text to decode
     * \param cp The next character
     * \return Pointer to after the next character
     * \author PH
     * \date 20071116
     */
    const char* decode_utf8(const char* inputString, uint32_t* cp);

    /*! \brief Replace all occurrences of 'searchForText' with 'replaceWithText' within 'originalString'.
     * 
     * @param originalString A copy of the original text string.
     * @param searchForText Substring to replace.
     * @param replaceWithText What to replace every 'searchForText' instance with.
     * @return Original string if no substitution made, or modified string.
     */
    std::string replaceAll(std::string originalString, const std::string& searchForText, const std::string& replaceWithText);

    /*! \brief Insert character names
     *
     * This checks a string for $0, or $1 and replaces with player names.
     *
     * PH 20030107 Increased limit on length of the_string.
     * NB. Values for $ other than $0 or $1 will cause errors.
     *
     * \param   the_string Input string
     * \returns string with player name incorporated.
     */
    std::string substitutePlayerNameString(const std::string& the_string);

    /*! \brief Split text into lines
     * \author PH
     * \date 20021220
     *
     *
     * Takes a string and re-formats it to fit into the msgbuf text buffer,
     * for displaying with generic_text().  Processes as much as it can to
     * fit in one box, and returns a pointer to the next unprocessed character
     *
     * \param   stringToSplit The string to reformat
     * \returns the rest of the string that has not been processed (will be empty if everything was processed).
     */
    std::string splitTextOverMultipleLines(const std::string& stringToSplit);

    /*! \brief Calculate bubble position
     *
     * The purpose of this function is to calculate where a text bubble
     * should go in relation to the entity who is speaking.
     *
     * \param   entity_index If value is between 0..MAX_ENTITIES (exclusive),
     *              character that is speaking, otherwise 'general'.
     */
    void set_textpos(uint32_t entity_index);

    /*! \brief Fade between sub-ranges of two palettes
     *
     *  Fades from source to dest, at the specified speed (1 is the slowest, 64
     *  is instantaneous). Only affects colors between 'from' and 'to' (inclusive,
     *  pass 0 and 255 to fade the entire palette).
     *  Calls poll_music() to ensure the song keeps playing smoothly.
     *  Based on the Allegro function of the same name.
     *
     * \param   source Palette to fade from
     * \param   dest Palette to fade to
     * \param   speed How fast to fade (1..64)
     * \param   from Starting palette index (0..255)
     * \param   to Ending palette index (0..255)
     * \date    20040731 PH added check for out-of-range speed
     */
    void _fade_from_range(AL_CONST PALETTE source, AL_CONST PALETTE dest, uint32_t speed, int from, int to);
};

extern KDraw kqDraw;
