#include "draw.h"

/**
 * Character and Map drawing
 *
 * Includes functions to draw characters, text and maps.
 * Also some color manipulation.
 */

#include "bounds.h"
#include "combat.h"
#include "console.h"
#include "constants.h"
#include "entity.h"
#include "gfx.h"
#include "input.h"
#include "kq.h"
#include "magic.h"
#include "music.h"
#include "res.h"
#include "setup.h"
#include "timing.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <sstream>
#include <string>

using std::string;

/* Globals */
namespace drawGlobals
{

constexpr uint32_t ScreenHeightInTiles = 16;
constexpr uint32_t ScreenWidthInTiles = 21;
constexpr uint32_t magic_number_216 = 216;
constexpr uint32_t magic_number_152 = 152;

constexpr size_t MSG_ROWS{ 4 };
constexpr size_t MSG_COLS{ 36 };

char msgbuf[MSG_ROWS][MSG_COLS];
string messageBuffer[MSG_ROWS];
int gbx, gby, gbbx, gbby, gbbw, gbbh, gbbs;
eBubbleStemStyle bubble_stem_style;

} // namespace drawGlobals

namespace
{

void setGbxy(const int gbx, const int gby)
{
    drawGlobals::gbx = gbx;
    drawGlobals::gby = gby;
}

void setGbx(const int gbx) { drawGlobals::gbx = gbx; }
int getGbx() { return drawGlobals::gbx; }
void setGby(const int gby) { drawGlobals::gby = gby; }
int getGby() { return drawGlobals::gby; }

void setGbbxy(const int gbbx, const int gbby)
{
    drawGlobals::gbbx = gbbx;
    drawGlobals::gbby = gbby;
}

void setGbbx(const int gbbx) { drawGlobals::gbbx = gbbx; }
int getGbbx() { return drawGlobals::gbbx; }
void setGbby(const int gbby) { drawGlobals::gbby = gbby; }
int getGbby() { return drawGlobals::gbby; }

void setGbbhw(const int gbbh, const int gbbw)
{
    drawGlobals::gbbh = gbbh;
    drawGlobals::gbbw = gbbw;
}

void setGbbh(const int gbbh) { drawGlobals::gbbh = gbbh; }
int getGbbh() { return drawGlobals::gbbh; }
void setGbbw(const int gbbw) { drawGlobals::gbbw = gbbw; }
int getGbbw() { return drawGlobals::gbbw; }

void setGbbs(const int gbbs) { drawGlobals::gbbs = gbbs; }
int getGbbs() { return drawGlobals::gbbs; }

void setBubbleStemStyle(const eBubbleStemStyle bubbleStemStyle) { drawGlobals::bubble_stem_style = bubbleStemStyle; }
eBubbleStemStyle getBubbleStemStyle() { return drawGlobals::bubble_stem_style; }

} // anonymous namespace

namespace KqFork
{
/*! \brief The internal processing modes during text reformatting
 *
 * \sa relay()
 */
enum class eTextFormat
{
    M_UNDEF,
    M_SPACE,
    M_NONSPACE,
    M_END
};

/**
 * Find the offset of the unicode glyph within the font.
 *
 * @param glyph Character code to look up, value 128..255
 * @return Offset within font file on success, or 0 if not found
 */
int glyphLookup(uint32_t glyph) {
    static const std::map<uint32_t, int> glyphMap = {
        { 0x00c9, 'E' - 32 }, /* E-acute */
        { 0x00d3, 'O' - 32 }, /* O-acute */
        { 0x00df, 107 },      /* sharp s */
        { 0x00e1, 92 },       /* a-grave */
        { 0x00e4, 94 },       /* a-umlaut */
        { 0x00e9, 95 },       /* e-acute */
        { 0x00ed, 'i' - 32 }, /* i-acute */
        { 0x00f1, 108 },      /* n-tilde */
        { 0x00f3, 99 },       /* o-acute */
        { 0x00f6, 102 },      /* o-umlaut */
        { 0x00fa, 103 },      /* u-acute */
        { 0x00fc, 106 },      /* u-umlaut */
    };
    auto glyphIter = glyphMap.find(glyph);
    if (glyphIter != glyphMap.end())
    {
        return glyphIter->second;
    }
    return 0;
}

} // namespace KqFork


void KDraw::blit2screen(int xw, int yw)
{
    static int frate = limit_frame_rate(25);

    if (show_frate == 1)
    {
        char fbuf[16];

        sprintf(fbuf, "%3d", frate);
        double_buffer->fill(xw, yw, xw + 24, yw + 8, 0);
        print_font(double_buffer, xw, yw, fbuf, eFontColor::FONTCOLOR_NORMAL);
    }
#ifdef DEBUGMODE
    display_console(xw, yw);
#endif
    acquire_screen();
    if (stretch_view == 1)
    {
        for (int j = 0; j < 480; ++j)
        {
            uint8_t* lptr = reinterpret_cast<uint8_t*>(bmp_write_line(screen, j));
            for (int i = 0; i < 640; i += 2)
            {
                lptr[i] = lptr[i + 1] = double_buffer->ptr(xw + i / 2, yw + j / 2);
            }
            bmp_unwrite_line(screen);
        }
    }
    else
    {
        for (int j = 0; j < 240; ++j)
        {
            uint8_t* lptr = reinterpret_cast<uint8_t*>(bmp_write_line(screen, j));
            for (int i = 0; i < 320; ++i)
            {
                lptr[i] = double_buffer->ptr(xw + i, yw + j);
            }
            bmp_unwrite_line(screen);
        }
    }
    release_screen();
    frate = limit_frame_rate(25);
}

void KDraw::border(Raster* where, int left, int top, int right, int bottom)
{
    // TODO: Find out whether these values paired to any color defined within
    // PALETTE 'pal'
    static const uint8_t GREY1 = 4;
    static const uint8_t GREY2 = 8;
    static const uint8_t GREY3 = 13;
    static const uint8_t WHITE = 15;

    hline(where, left  + 0, bottom - 0, right  - 0, GREY1);
    vline(where, left  + 0, top    + 0, bottom - 0, WHITE);

    vline(where, left  + 1, top + 3, bottom - 3, GREY2);
    vline(where, right - 2, top + 3, bottom - 3, GREY2);

    vline(where, left  + 2, top + 3, bottom - 3, GREY3);
    vline(where, right - 3, top + 3, bottom - 3, GREY3);

    vline(where, left  + 3, top + 2, bottom - 2, GREY3);
    vline(where, right - 4, top + 2, bottom - 2, GREY3);

    vline(where, left  + 3, top + 5, bottom - 5, WHITE);
    vline(where, right - 4, top + 5, bottom - 5, WHITE);

    vline(where, left  + 4, top + 5, bottom - 5, GREY1);
    vline(where, right - 5, top + 5, bottom - 5, GREY1);

    hline(where, left + 3, top    + 1, right - 3, GREY2);
    hline(where, left + 3, bottom - 2, right - 3, GREY2);

    hline(where, left + 3, top    + 2, right - 3, GREY3);
    hline(where, left + 3, bottom - 3, right - 3, GREY3);

    hline(where, left + 4, top    + 3, right - 4, GREY3);
    hline(where, left + 4, bottom - 4, right - 4, GREY3);

    hline(where, left + 5, top    + 3, right - 5, WHITE);
    hline(where, left + 5, bottom - 4, right - 5, WHITE);

    hline(where, left + 5, top    + 4, right - 5, GREY1);
    hline(where, left + 5, bottom - 5, right - 5, GREY1);

    putpixel(where, left + 2, top    + 2, GREY2);
    putpixel(where, left + 2, bottom - 3, GREY2);

    putpixel(where, right - 3, top    + 2, GREY2);
    putpixel(where, right - 3, bottom - 3, GREY2);

    putpixel(where, left + 4, top    + 4, WHITE);
    putpixel(where, left + 4, bottom - 5, WHITE);

    putpixel(where, right - 5, top    + 4, WHITE);
    putpixel(where, right - 5, bottom - 5, WHITE);
}

void KDraw::color_scale(Raster* src, Raster* dest, int output_range_start, int output_range_end)
{
    if (src == nullptr || dest == nullptr)
    {
        return;
    }

    clear_bitmap(dest);
    for (uint16_t iy = 0; iy < dest->height; iy++)
    {
        for (uint16_t ix = 0; ix < dest->width; ix++)
        {
            uint8_t current_pixel_color = src->getpixel(ix, iy);
            if (current_pixel_color > 0)
            {
                uint8_t z = pal[current_pixel_color].r;
                z += pal[current_pixel_color].g;
                z += pal[current_pixel_color].b;
                // 192 is '64*3' (max value for each of R, G and B).
                z = z * (output_range_end - output_range_start) / 192;
                dest->setpixel(ix, iy, output_range_start + z);
            }
        }
    }
}

void KDraw::convert_cframes(size_t fighter_index, int output_range_start, int output_range_end, int convert_heroes)
{
    uint32_t start_fighter_index = 0;
    uint32_t end_fighter_index = 0;

    /* Determine the range of frames to convert */
    if (convert_heroes == 1)
    {
        if (fighter_index < MAX_PARTY_SIZE)
        {
            start_fighter_index = 0;
            end_fighter_index = numchrs;
        }
        else
        {
            start_fighter_index = MAX_PARTY_SIZE;
            end_fighter_index = MAX_PARTY_SIZE + kqCombat.num_enemies;
        }
    }
    else
    {
        start_fighter_index = fighter_index;
        end_fighter_index = fighter_index + 1;
    }

    while (start_fighter_index < end_fighter_index)
    {
        for (size_t cframe_index = 0; cframe_index < MAXCFRAMES; cframe_index++)
        {
            color_scale(tcframes[start_fighter_index][cframe_index], cframes[start_fighter_index][cframe_index], output_range_start, output_range_end);
        }
        ++start_fighter_index;
    }
}

Raster* KDraw::copy_bitmap(Raster* target, Raster* source)
{
    if (source == nullptr)
    {
        return nullptr;
    }

    if (target != nullptr)
    {
        if (target->width < source->width || target->height < source->height)
        {
            /* too small */
            delete (target);
            target = new Raster(source->width, source->height);
        }
    }
    else
    {
        /* create new */
        target = new Raster(source->width, source->height);
    }
    /* ...and copy */
    source->blitTo(target);
    return target;
}

void KDraw::_fade_from_range(AL_CONST PALETTE source, AL_CONST PALETTE dest, uint32_t speed, int from, int to)
{
    PALETTE temp;
    int c, start, last;
    size_t pal_index;

    /* make sure fade speed is in range */
    if (speed < 1)
    {
        speed = 1;
    }
    if (speed > 64)
    {
        speed = 64;
    }

    for (pal_index = 0; pal_index < PAL_SIZE; pal_index++)
    {
        temp[pal_index] = source[pal_index];
    }
    start = retrace_count;
    last = -1;
    while ((c = (retrace_count - start) * speed / 2) < 64)
    {
        Music.poll_music();
        if (c != last)
        {
            fade_interpolate(source, dest, temp, c, from, to);
            set_palette_range(temp, from, to, TRUE);
            if (_color_depth > 8)
            {
                kqDraw.blit2screen(xofs, yofs);
            }
            last = c;
        }
    }
    set_palette_range(dest, from, to, TRUE);
}

void KDraw::do_transition(eTransitionFade transitionFade, int transitionSpeed)
{
    switch (transitionFade)
    {
        case eTransitionFade::TRANS_FADE_IN:
        {
            _fade_from_range(black_palette, pal, transitionSpeed, 0, PAL_SIZE - 1);
            break;
        }
        case eTransitionFade::TRANS_FADE_OUT:
        {
            PALETTE temp;

            get_palette(temp);
            _fade_from_range(temp, black_palette, transitionSpeed, 0, PAL_SIZE - 1);
            break;
        }
        case eTransitionFade::TRANS_FADE_WHITE:
        {
            PALETTE temp, whp;

            get_palette(temp);
            for (size_t a = 0; a < 256; a++)
            {
                whp[a].r = 63;
                whp[a].g = 63;
                whp[a].b = 63;
            }
            _fade_from_range(temp, whp, transitionSpeed, 0, PAL_SIZE - 1);
            break;
        }
        default:
            break;
    }
}

void KDraw::draw_backlayer()
{
    int dx, dy, xtc, ytc;
    uint16_t pix;
    int here;
    KBound box;

    if (view_on == 0)
    {
        view_y1 = 0;
        view_y2 = g_map.ysize - 1;
        view_x1 = 0;
        view_x2 = g_map.xsize - 1;
    }
    if (g_map.map_mode < 2 || g_map.map_mode > 3)
    {
        xtc = camera_viewport_x >> 4;
        ytc = camera_viewport_y >> 4;
        dx = camera_viewport_x;
        dy = camera_viewport_y;
        box.left = view_x1;
        box.top = view_y1;
        box.right = view_x2;
        box.bottom = view_y2;
    }
    else
    {
        // For divide-by-zero
        if (g_map.pdiv == 0)
        {
            g_map.pdiv = 1;
        }

        dx = camera_viewport_x * g_map.pmult / g_map.pdiv;
        dy = camera_viewport_y * g_map.pmult / g_map.pdiv;
        xtc = dx / TILE_W;
        ytc = dy / TILE_H;
        box.left = view_x1 * g_map.pmult / g_map.pdiv;
        box.top = view_y1 * g_map.pmult / g_map.pdiv;
        box.right = view_x2 * g_map.pmult / g_map.pdiv;
        box.bottom = view_y2 * g_map.pmult / g_map.pdiv;
    }
    xofs = 16 - (dx & 15);
    yofs = 16 - (dy & 15);

    for (dy = 0; dy < drawGlobals::ScreenHeightInTiles; dy++)
    {
        /* TT Parallax problem here #1 */
        if (ytc + dy >= box.top && ytc + dy <= box.bottom)
        {
            for (dx = 0; dx < drawGlobals::ScreenWidthInTiles; dx++)
            {
                /* TT Parallax problem here #2 */
                if (xtc + dx >= box.left && xtc + dx <= box.right)
                {
                    here = ((ytc + dy) * g_map.xsize) + xtc + dx;
                    pix = map_seg[here];
                    blit(map_icons[tilex[pix]], double_buffer, 0, 0, dx * 16 + xofs, dy * 16 + yofs, 16, 16);
                }
            }
        }
    }
}

void KDraw::draw_char()
{
    // Draw back-to-front characters so the lead character is rendered over all others.
    for (size_t follower_fighter_index = MAX_PARTY_SIZE + kEntity.getNumberOfMapEntities(); follower_fighter_index > 0; follower_fighter_index--)
    {
        size_t fighter_frame = 0;
        size_t fighter_index = follower_fighter_index - 1;
        auto& follower_ent = g_ent[fighter_index];
        int32_t dx = follower_ent.x - camera_viewport_x + TILE_W;
        int32_t dy = follower_ent.y - camera_viewport_y + TILE_H;

        if (!follower_ent.moving)
        {
            fighter_frame = follower_ent.facing * ENT_FRAMES_PER_DIR + 2;
        }
        else if (follower_ent.framectr >= MAX_FRAMECTR / 2)
        {
            // Fighter is moving
            fighter_frame = follower_ent.facing * ENT_FRAMES_PER_DIR + 1;
        }
        else
        {
            // Fighter is moving
            fighter_frame = follower_ent.facing * ENT_FRAMES_PER_DIR + 0;
        }

        if (fighter_index < MAX_PARTY_SIZE && fighter_index < numchrs)
        {
            render_hero(fighter_index, fighter_frame);
        }
        else
        {
            render_npc(fighter_index, dx, dy, fighter_frame);
        }
    }
}

void KDraw::render_npc(size_t fighter_index, int32_t dx, int32_t dy, size_t fighter_frame)
{
    auto& npc_entity = g_ent[fighter_index];

    if (npc_entity.active &&
        npc_entity.tilex >= view_x1 &&
        npc_entity.tilex <= view_x2 &&
        npc_entity.tiley >= view_y1 &&
        npc_entity.tiley <= view_y2)
    {
        int32_t tileW = static_cast<int32_t>(TILE_W);
        int32_t tileH = static_cast<int32_t>(TILE_H);
        int32_t onscreenTilesW = static_cast<int32_t>(ONSCREEN_TILES_W);
        int32_t onscreenTilesH = static_cast<int32_t>(ONSCREEN_TILES_H);
        if (dx >= -tileW && dx <= tileW * (onscreenTilesW + 1) &&
            dy >= -tileH && dy <= tileH * (onscreenTilesH + 1))
        {
            Raster* spr = (npc_entity.eid >= ID_ENEMY)
                ? eframes[npc_entity.identity()][fighter_frame]
                : frames[npc_entity.eid][fighter_frame];

            if (!npc_entity.isSemiTransparent)
            {
                draw_sprite(double_buffer, spr, dx, dy);
            }
            else
            {
                draw_trans_sprite(double_buffer, spr, dx, dy);
            }
        }
    }
}

void KDraw::render_hero(size_t fighter_index, size_t fighter_frame)
{
    size_t fighter_type_id = g_ent[fighter_index].eid;
    Raster** sprite_base = nullptr;
    auto& hero_entity = g_ent[fighter_index];
    Raster* spr = nullptr;

    /* It's a hero */
    /* Masquerade: if chrx!=0 then this hero is disguised as someone else...
     */
    sprite_base = hero_entity.identity()
        ? eframes[hero_entity.identity()]
        : frames[fighter_type_id];

    if (party[fighter_type_id].sts[S_DEAD] == eDeathType::IS_DEAD)
    {
        fighter_frame = hero_entity.facing * ENT_FRAMES_PER_DIR + 2;
    }

    if (party[fighter_type_id].sts[S_POISON] != 0)
    {
        /* PH: we are calling this every frame? */
        color_scale(sprite_base[fighter_frame], tc2, 32, 47);
        spr = tc2;
    }
    else
    {
        spr = sprite_base[fighter_frame];
    }
    if (is_forestsquare(g_ent[fighter_index].tilex, g_ent[fighter_index].tiley))
    {
        bool f = !g_ent[fighter_index].moving;
        if (g_ent[fighter_index].moving && is_forestsquare(g_ent[fighter_index].x / TILE_W, g_ent[fighter_index].y / TILE_H))
        {
            f = true;
        }
        if (f)
        {
            clear_to_color(tc, 0);
            blit(spr, tc, 0, 0, 0, 0, 16, 6);
            spr = tc;
        }
    }

    int32_t dx = g_ent[fighter_index].x - camera_viewport_x + TILE_W;
    int32_t dy = g_ent[fighter_index].y - camera_viewport_y + TILE_H;
    if (party[fighter_type_id].sts[S_DEAD] == eDeathType::NOT_DEAD)
    {
        draw_sprite(double_buffer, spr, dx, dy);
    }
    else
    {
        draw_trans_sprite(double_buffer, spr, dx, dy);
    }

    /* After we draw the player's character, we have to know whether they
     * are moving diagonally. If so, we need to draw both layers 1&2 on
     * the correct tile, which helps correct diagonal movement artifacts.
     * We also need to ensure that the target coords has SOMETHING in the
     * o_seg[] portion, else there will be graphical glitches.
     */
    if (fighter_index == 0 && g_ent[0].moving)
    {
        int32_t horiz = 0;
        int32_t vert = 0;
        /* Determine the direction moving */

        if (g_ent[fighter_index].tilex * TILE_W > g_ent[fighter_index].x)
        {
            horiz = 1; // Right
        }
        else if (g_ent[fighter_index].tilex * TILE_W < g_ent[fighter_index].x)
        {
            horiz = -1; // Left
        }

        if (g_ent[fighter_index].tiley * TILE_H > g_ent[fighter_index].y)
        {
            vert = 1; // Down
        }
        else if (g_ent[fighter_index].tiley * TILE_H < g_ent[fighter_index].y)
        {
            vert = -1; // Up
        }

        /* Moving diagonally means both horiz and vert are non-zero */
        if (horiz != 0 && vert != 0)
        {
            int x = 0;
            int y = 0;
            uint32_t here = 0;
            uint32_t there = 0;

            /* When moving down, we will draw over the spot directly below
             * our starting position. Since tile[xy] shows our final coord,
             * we will instead draw to the left or right of the final pos.
             */
            if (vert > 0)
            {
                /* Moving diag down */

                // Final x-coord is one left/right of starting x-coord
                x = (g_ent[fighter_index].tilex - horiz) * TILE_W - camera_viewport_x + TILE_W;
                // Final y-coord is same as starting y-coord
                y = g_ent[fighter_index].tiley * TILE_H - camera_viewport_y + TILE_H;
                // Where the tile is on the map that we will draw over
                there = (g_ent[fighter_index].tiley) * g_map.xsize + g_ent[fighter_index].tilex - horiz;
                // Original position, before you started moving
                here = (g_ent[fighter_index].tiley - vert) * g_map.xsize + g_ent[fighter_index].tilex - horiz;
            }
            else
            {
                /* Moving diag up */

                // Final x-coord is same as starting x-coord
                x = g_ent[fighter_index].tilex * TILE_W - camera_viewport_x + TILE_W;
                // Final y-coord is above starting y-coord
                y = (g_ent[fighter_index].tiley - vert) * TILE_H - camera_viewport_y + TILE_H;
                // Where the tile is on the map that we will draw over
                there = (g_ent[fighter_index].tiley - vert) * g_map.xsize + g_ent[fighter_index].tilex;
                // Target position
                here = (g_ent[fighter_index].tiley) * g_map.xsize + g_ent[fighter_index].tilex;
            }

            /* Because of possible redraw problems, only draw if there is
             * something drawn over the player (f_seg[] != 0)
             */
            if (tilex[f_seg[here]] != 0)
            {
                draw_sprite(double_buffer, map_icons[tilex[map_seg[there]]], x, y);
                draw_sprite(double_buffer, map_icons[tilex[b_seg[there]]], x, y);
            }
        }
    }
}

void KDraw::draw_forelayer()
{
    int dx, dy, pix, xtc, ytc;
    int here;
    KBound box;

    if (view_on == 0)
    {
        view_y1 = 0;
        view_y2 = g_map.ysize - 1;
        view_x1 = 0;
        view_x2 = g_map.xsize - 1;
    }
    if (g_map.map_mode < 4 || g_map.pdiv == 0)
    {
        dx = camera_viewport_x;
        dy = camera_viewport_y;
        box.left = view_x1;
        box.top = view_y1;
        box.right = view_x2;
        box.bottom = view_y2;
    }
    else
    {
        dx = camera_viewport_x * g_map.pmult / g_map.pdiv;
        dy = camera_viewport_y * g_map.pmult / g_map.pdiv;
        box.left = view_x1 * g_map.pmult / g_map.pdiv;
        box.top = view_y1 * g_map.pmult / g_map.pdiv;
        box.right = view_x2 * g_map.pmult / g_map.pdiv;
        box.bottom = view_y2 * g_map.pmult / g_map.pdiv;
    }
    xtc = dx / TILE_W;
    ytc = dy / TILE_H;

    xofs = 16 - (dx & 15);
    yofs = 16 - (dy & 15);

    for (dy = 0; dy < drawGlobals::ScreenHeightInTiles; dy++)
    {
        if (ytc + dy >= box.top && ytc + dy <= box.bottom)
        {
            for (dx = 0; dx < drawGlobals::ScreenWidthInTiles; dx++)
            {
                if (xtc + dx >= box.left && xtc + dx <= box.right)
                {
                    // Used in several places in this loop, so shortened the name
                    here = ((ytc + dy) * g_map.xsize) + xtc + dx;
                    pix = f_seg[here];
                    draw_sprite(double_buffer, map_icons[tilex[pix]], dx * 16 + xofs, dy * 16 + yofs);

#ifdef DEBUGMODE
                    if (debugging > 3)
                    {
                        // Obstacles
                        if (o_seg[here] == 1)
                        {
                            draw_sprite(double_buffer, obj_mesh, dx * 16 + xofs, dy * 16 + yofs);
                        }

// Zones
#if (ALLEGRO_VERSION >= 4 && ALLEGRO_SUB_VERSION >= 1)
                        if (z_seg[here] == 0)
                        {
                            // Do nothing
                        }
                        else
                        {
                            char buf[8];
                            sprintf(buf, "%d", z_seg[here]);
                            size_t l = strlen(buf) * 8;
                            print_num(double_buffer, dx * 16 + 8 + xofs - l / 2, dy * 16 + 4 + yofs, buf, eFont::FONT_WHITE);
                        }
#else
                        if (z_seg[here] == 0)
                        {
                            // Do nothing
                        }
                        else if (z_seg[here] < 10)
                        {
                            /* The zone's number is single-digit, center vert+horiz */
                            textprintf(double_buffer, font, dx * 16 + 4 + xofs, dy * 16 + 4 + yofs, makecol(255, 255, 255), "%d", z_seg[here]);
                        }
                        else if (z_seg[here] < 100)
                        {
                            /* The zone's number is double-digit, center only vert */
                            textprintf(double_buffer, font, dx * 16 + xofs, dy * 16 + 4 + yofs, makecol(255, 255, 255), "%d", z_seg[here]);
                        }
                        else if (z_seg[here] < 10)
                        {
                            /* The zone's number is triple-digit.  Print the 100's
                             * digit in top-center of the square; the 10's and 1's
                             * digits on bottom of the square
                             */
                            textprintf(double_buffer, font, dx * 16 + 4 + xofs, dy * 16 + yofs, makecol(255, 255, 255), "%d", (int)(z_seg[here] / 100));
                            textprintf(double_buffer, font, dx * 16 + xofs, dy * 16 + 8 + yofs, makecol(255, 255, 255), "%02d", (int)(z_seg[here] % 100));
                        }
#endif /* (ALLEGRO_VERSION) */
                    }
#endif /* DEBUGMODE */
                }
            }
        }
    }
}

void KDraw::draw_icon(Raster* where, int ino, int icx, int icy)
{
    masked_blit(sicons, where, 0, ino * 8, icx, icy, 8, 8);
}

void KDraw::draw_kq_box(Raster* where, int x1, int y1, int x2, int y2, eMenuBoxColor bgColor, eBubbleStyle bstyle)
{
    // This is mapped to the color palette
    static const uint8_t DBLUE = 3;
    static const uint8_t DRED = 6;

    int colorPaletteBgColor = 0;

    switch (bgColor) {
    case eMenuBoxColor::SEMI_TRANSPARENT_BLUE:
        colorPaletteBgColor = bgColor;

        /* Draw a maybe-translucent background */
        drawing_mode(DRAW_MODE_TRANS, nullptr, 0, 0);
        break;
    case eMenuBoxColor::DARKBLUE:
        colorPaletteBgColor = DBLUE;
        break;
    default:
        colorPaletteBgColor = DRED;
        break;
    }

    rectfill(where, x1 + 2, y1 + 2, x2 - 3, y2 - 3, colorPaletteBgColor);
    drawing_mode(DRAW_MODE_SOLID, nullptr, 0, 0);
    /* Now the border */
    switch (bstyle)
    {
    case eBubbleStyle::BUBBLE_TEXT:
    case eBubbleStyle::BUBBLE_MESSAGE:
        border(where, x1, y1, x2 - 1, y2 - 1);
        break;

    case eBubbleStyle::BUBBLE_THOUGHT:
        /* top and bottom */
        for (int a = x1 + 8; a < x2 - 8; a += 8)
        {
            draw_sprite(where, bord[1], a, y1);
            draw_sprite(where, bord[6], a, y2 - 8);
        }
        /* sides */
        for (int a = y1 + 8; a < y2 - 8; a += 12)
        {
            draw_sprite(where, bord[3], x1, a);
            draw_sprite(where, bord[4], x2 - 8, a);
        }
        /* corners */
        draw_sprite(where, bord[0], x1, y1);
        draw_sprite(where, bord[2], x2 - 8, y1);
        draw_sprite(where, bord[5], x1, y2 - 8);
        draw_sprite(where, bord[7], x2 - 8, y2 - 8);
        break;

    default: /* no border */
        break;
    }
}

void KDraw::draw_midlayer()
{
    int dx, dy, pix, xtc, ytc;
    int here;
    KBound box;

    if (view_on == 0)
    {
        view_y1 = 0;
        view_y2 = g_map.ysize - 1;
        view_x1 = 0;
        view_x2 = g_map.xsize - 1;
    }
    if (g_map.map_mode < 3 || g_map.map_mode == 5)
    {
        xtc = camera_viewport_x >> 4;
        ytc = camera_viewport_y >> 4;
        dx = camera_viewport_x;
        dy = camera_viewport_y;
        box.left = view_x1;
        box.top = view_y1;
        box.right = view_x2;
        box.bottom = view_y2;
    }
    else
    {
        // For divide-by-zero
        if (g_map.pdiv == 0)
        {
            g_map.pdiv = 1;
        }

        dx = camera_viewport_x * g_map.pmult / g_map.pdiv;
        dy = camera_viewport_y * g_map.pmult / g_map.pdiv;
        xtc = dx / TILE_W;
        ytc = dy / TILE_H;
        box.left = view_x1 * g_map.pmult / g_map.pdiv;
        box.top = view_y1 * g_map.pmult / g_map.pdiv;
        box.right = view_x2 * g_map.pmult / g_map.pdiv;
        box.bottom = view_y2 * g_map.pmult / g_map.pdiv;
    }
    xofs = 16 - (dx & 15);
    yofs = 16 - (dy & 15);

    for (dy = 0; dy < drawGlobals::ScreenHeightInTiles; dy++)
    {
        if (ytc + dy >= box.top && ytc + dy <= box.bottom)
        {
            for (dx = 0; dx < drawGlobals::ScreenWidthInTiles; dx++)
            {
                if (xtc + dx >= box.left && xtc + dx <= box.right)
                {
                    here = ((ytc + dy) * g_map.xsize) + xtc + dx;
                    pix = b_seg[here];
                    draw_sprite(double_buffer, map_icons[tilex[pix]], dx * 16 + xofs, dy * 16 + yofs);
                }
            }
        }
    }
}

void KDraw::draw_playerbound()
{
    int dx, dy, xtc, ytc;
    std::shared_ptr<KBound> found = nullptr;
    uint16_t ent_x = g_ent[0].tilex;
    uint16_t ent_y = g_ent[0].tiley;

    /* Is the player standing inside a bounding area? */
    uint32_t found_index = g_map.bounds.IsBound(ent_x, ent_y, ent_x, ent_y);
    if (!found_index)
    {
        return;
    }
    found = g_map.bounds.GetBound(found_index - 1);

    xtc = camera_viewport_x >> 4;
    ytc = camera_viewport_y >> 4;

    xofs = 16 - (camera_viewport_x & 15);
    yofs = 16 - (camera_viewport_y & 15);

    /* If the player is inside the bounded area, draw everything OUTSIDE the
     * bounded area with the tile specified by that area.
     * found->btile is most often 0, but could also be made to be water, etc.
     */

    // Top
    for (dy = 0; dy < found->top - ytc; dy++)
    {
        for (dx = 0; dx <= ONSCREEN_TILES_W; dx++)
        {
            blit(map_icons[tilex[found->btile]], double_buffer, 0, 0, dx * TILE_W + xofs, dy * TILE_H + yofs, TILE_W, TILE_H);
        }
    }

    // Sides
    for (dy = found->top - ytc; dy < found->bottom - ytc + 1; dy++)
    {
        // Left side
        for (dx = 0; dx < found->left - xtc; dx++)
        {
            blit(map_icons[tilex[found->btile]], double_buffer, 0, 0, dx * TILE_W + xofs, dy * TILE_H + yofs, TILE_W, TILE_H);
        }

        // Right side
        for (dx = found->right - xtc + 1; dx <= ONSCREEN_TILES_W; dx++)
        {
            blit(map_icons[tilex[found->btile]], double_buffer, 0, 0, dx * TILE_W + xofs, dy * TILE_H + yofs, TILE_W, TILE_H);
        }
    }

    // Bottom
    for (dy = found->bottom - ytc + 1; dy <= ONSCREEN_TILES_H; dy++)
    {
        for (dx = 0; dx <= ONSCREEN_TILES_W; dx++)
        {
            blit(map_icons[tilex[found->btile]], double_buffer, 0, 0, dx * TILE_W + xofs, dy * TILE_H + yofs, TILE_W, TILE_H);
        }
    }
}

void KDraw::draw_shadows()
{
    int dx, dy, pix, xtc, ytc;
    int here;

    if (draw_shadow == 0)
    {
        return;
    }
    if (!view_on)
    {
        view_y1 = 0;
        view_y2 = g_map.ysize - 1;
        view_x1 = 0;
        view_x2 = g_map.xsize - 1;
    }
    xtc = camera_viewport_x >> 4;
    ytc = camera_viewport_y >> 4;
    xofs = 16 - (camera_viewport_x & 15);
    yofs = 16 - (camera_viewport_y & 15);

    for (dy = 0; dy < drawGlobals::ScreenHeightInTiles; dy++)
    {
        for (dx = 0; dx < drawGlobals::ScreenWidthInTiles; dx++)
        {
            if (ytc + dy >= view_y1 && xtc + dx >= view_x1 &&
                ytc + dy <= view_y2 && xtc + dx <= view_x2)
            {
                here = ((ytc + dy) * g_map.xsize) + xtc + dx;
                pix = s_seg[here];
                if (pix > 0)
                {
                    draw_trans_sprite(double_buffer, shadow[pix], dx * 16 + xofs, dy * 16 + yofs);
                }
            }
        }
    }
}

void KDraw::draw_stsicon(Raster* where, int cc, int who, uint32_t inum, int icx, int icy)
{
    int st = 0;

    for (size_t statIdx = 0; statIdx < inum; ++statIdx)
    {
        const int s = (cc == 0) ?
            party[who].sts[statIdx] :
            fighter[who].fighterSpellEffectStats[statIdx];

        if (s != 0)
        {
            masked_blit(stspics, where, 0, statIdx * 8 + 8, st * 8 + icx, icy, 8, 8);
            ++st;
        }
    }
    if (st == 0)
    {
        masked_blit(stspics, where, 0, 0, icx, icy, 8, 8);
    }
}

void KDraw::draw_textbox(eBubbleStyle bstyle)
{
    const int wid = getGbbw() * 8 + 16;
    const int hgt = getGbbh() * 12 + 16;
    const int gbbxOfs = getGbbx() + xofs;
    const int gbbyOfs = getGbby() + yofs;

    draw_kq_box(double_buffer, gbbxOfs, gbbyOfs, gbbxOfs + wid, gbbyOfs + hgt, eMenuBoxColor::SEMI_TRANSPARENT_BLUE, bstyle);
    if (getBubbleStemStyle() != eBubbleStemStyle::STEM_UNDEFINED)
    {
        /* select the correct stem-thingy that comes out of the speech bubble */
        Raster* stem = bub[getBubbleStemStyle() + (bstyle == eBubbleStyle::BUBBLE_THOUGHT ? eBubbleStemStyle::NUM_BUBBLE_STEMS : 0)];

        /* and draw it */
        if (stem != nullptr)
        {
            draw_sprite(double_buffer, stem, getGbx() + xofs, getGby() + yofs);
    }
    }

    for (int i = 0, iMax = getGbbh(); i < iMax; ++i)
    {
        print_font(double_buffer,
                   gbbxOfs + 8,
                   i * 12 + gbbyOfs + 8,
                   drawGlobals::messageBuffer[i].c_str(),
                   eFontColor::FONTCOLOR_BIG);
    }
}

void KDraw::draw_porttextbox(eBubbleStyle bstyle, int chr)
{
    const int gbbxOfs = getGbbx() + xofs;
    const int gbbyOfs = getGbby() + yofs;

    int wid, hgt, a;
    int linexofs;

    wid = getGbbw() * 8 + 16;
    hgt = getGbbh() * 12 + 16;
    chr = chr - MAX_PARTY_SIZE;

    draw_kq_box(double_buffer, gbbxOfs, gbbyOfs, gbbxOfs + wid, gbbyOfs + hgt, eMenuBoxColor::SEMI_TRANSPARENT_BLUE, bstyle);

    for (a = 0; a < getGbbh(); a++)
    {
        print_font(double_buffer, gbbxOfs + 8, a * 12 + gbbyOfs + 8, drawGlobals::messageBuffer[a].c_str(), eFontColor::FONTCOLOR_BIG);
    }

    a--;
    linexofs = a * 12;

    menubox(double_buffer, 19, 172 - linexofs, 4, 4, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
    menubox(double_buffer, 66, 196 - linexofs, party[chr].playerName.length(), 1, eMenuBoxColor::SEMI_TRANSPARENT_BLUE);

    draw_sprite(double_buffer, players[chr].portrait, 24, 177 - linexofs);
    print_font(double_buffer, 74, 204 - linexofs, party[chr].playerName.c_str(), eFontColor::FONTCOLOR_NORMAL);
}

void KDraw::drawmap()
{
    if (g_map.xsize <= 0)
    {
        clear_to_color(double_buffer, 1);
        return;
    }
    clear_bitmap(double_buffer);
    if (draw_background)
    {
        draw_backlayer();
    }
    if (g_map.map_mode == 1 || g_map.map_mode == 3 || g_map.map_mode == 5)
    {
        draw_char();
    }
    if (draw_middle)
    {
        draw_midlayer();
    }
    if (g_map.map_mode == 0 || g_map.map_mode == 2 || g_map.map_mode == 4)
    {
        draw_char();
    }
    if (draw_foreground)
    {
        draw_forelayer();
    }
    draw_shadows();
    draw_playerbound();

    /*  This is an obvious hack here.  When I first started, xofs and yofs could
     *  have values of anywhere between 0 and 15.  Therefore, I had to use these
     *  offsets any time I drew to the double_buffer.  However, when I put in the
     *  parallaxing code, that was no longer true.  So, instead of changing all
     *  my code, I just put this hack in place.  It's actually kind of handy in
     *  case I ever have to adjust stuff again.
     */
    xofs = 16;
    yofs = 16;
    if (save_spells[P_REPULSE] > 0)
    {
        rectfill(b_repulse, 0, 16, 15, 165, 0);
        rectfill(b_repulse, 5, 16, 10, 16 + save_spells[P_REPULSE], 15);
        draw_trans_sprite(double_buffer, b_repulse, 2 + xofs, 2 + yofs);
    }
    if (display_desc == 1)
    {
        const size_t textLen = g_map.map_desc.length();
        menubox(double_buffer,
                drawGlobals::magic_number_152 - (textLen * 4) + xofs,
                8 + yofs,
                textLen,
                1,
                eMenuBoxColor::SEMI_TRANSPARENT_BLUE);
        print_font(double_buffer,
                   160 - (textLen * 4) + xofs,
                   16 + yofs,
                   g_map.map_desc.c_str(),
                   eFontColor::FONTCOLOR_NORMAL);
    }
}

void KDraw::generic_text(int who, eBubbleStyle box_style, int isPort)
{
    int a, stop = 0;

    setGbbhw(0, 1);
    setGbbs(0);
    for (a = 0; a < 4; a++)
    {
        const size_t len = drawGlobals::messageBuffer[a].length();
        /* FIXME: PH changed >1 to >0 */
        if (len > 0)
        {
            setGbbh(a + 1);
            if ((signed int)len > getGbbw())
            {
                setGbbw(len);
            }
        }
    }

    set_textpos((box_style == eBubbleStyle::BUBBLE_MESSAGE) ? -1 : (isPort == 0) ? who : 255);
    if (getGbbw() == -1 || getGbbh() == -1)
    {
        return;
    }
    Game.unpress();
    timer_count = 0;
    while (!stop)
    {
        Game.do_check_animation();
        drawmap();
        if (isPort == 0)
        {
            draw_textbox(box_style);
        }
        else
        {
            draw_porttextbox(box_style, who);
        }
        blit2screen(xofs, yofs);
        PlayerInput.readcontrols();
        if (PlayerInput.balt)
        {
            Game.unpress();
            stop = 1;
        }
    }
    timer_count = 0;
}

int KDraw::is_forestsquare(int fx, int fy)
{
    if (!Game.IsOverworldMap())
    {
        return 0;
    }
    auto mapseg = map_seg[(fy * g_map.xsize) + fx];
    switch (mapseg)
    {
    case 63:
    case 65:
    case 66:
    case 67:
    case 71:
    case 72:
    case 73:
    case 74:
        return 1;

    default:
        return 0;
    }
}

void KDraw::menubox(Raster* where, int x, int y, int w, int h, eMenuBoxColor bgColor)
{
    draw_kq_box(where, x, y, x + w * 8 + TILE_W, y + h * 8 + TILE_H, bgColor, eBubbleStyle::BUBBLE_TEXT);
}

void KDraw::message(const char* m, int icn, int delay, int x_m, int y_m)
{
    char msg[1024];
    const char* s = nullptr;
    size_t i, num_lines, max_len;

    /* Do the $0 replacement stuff */
    memset(msg, 0, sizeof(msg));
    strncpy(msg, substitutePlayerNameString(m).c_str(), sizeof(msg) - 1);
    s = msg;

    /* Save a copy of the screen */
    blit(double_buffer, back, x_m, y_m, 0, 0, SCREEN_W2, SCREEN_H2);

    /* Loop for each box full of text... */
    while (s != nullptr)
    {
        s = splitTextOverMultipleLines(s).c_str();
        /* Calculate the box size */
        num_lines = max_len = 0;
        for (i = 0; i < drawGlobals::MSG_ROWS; ++i)
        {
            size_t len = drawGlobals::messageBuffer[i].length();
            if (len > 0)
            {
                if (max_len < len)
                {
                    max_len = len;
                }
                ++num_lines;
            }
        }
        /* Draw the box and maybe the icon */
        if (icn == 255)
        {
            /* No icon */
            menubox(double_buffer, drawGlobals::magic_number_152 - (max_len * 4) + x_m, 108 + y_m, max_len, num_lines, eMenuBoxColor::DARKBLUE);
        }
        else
        {
            /* There is an icon; make the box a little bit bigger to the left */
            menubox(double_buffer, 144 - (max_len * 4) + x_m, 108 + y_m, max_len + 1, num_lines, eMenuBoxColor::DARKBLUE);
            draw_icon(double_buffer, icn, drawGlobals::magic_number_152 - (max_len * 4) + x_m, 116 + y_m);
        }

        /* Draw the text */
        for (i = 0; i < num_lines; ++i)
        {
            print_font(double_buffer, 160 - (max_len * 4) + x_m, 116 + 8 * i + y_m, drawGlobals::messageBuffer[i].c_str(), eFontColor::FONTCOLOR_NORMAL);
        }
        /* Show it */
        blit2screen(x_m, y_m);
        /* Wait for delay time or key press */
        if (delay == 0)
        {
            Game.wait_enter();
        }
        else
        {
            kq_wait(delay);
        }
        blit(back, double_buffer, 0, 0, x_m, y_m, SCREEN_W2, SCREEN_H2);
    }
}

string KDraw::replaceAll(string originalString, const string& searchForText, const string& replaceWithText)
{
    size_t startPosition = originalString.find(searchForText, 0);
    while (startPosition != string::npos)
    {
        originalString.replace(startPosition, searchForText.length(), replaceWithText);
        startPosition += replaceWithText.length(); // Handles case where 'to' is a substring of 'from'
        startPosition = originalString.find(searchForText, startPosition);
    }
    return originalString;
}

// TODO(onlinecop): Potential bug: if player changes order of party members and this
// was previously filled, it may not be updated to reflect that.
string getPlayerName(const size_t playerId)
{
    static std::map<size_t, string> playerNames;
    if (playerNames.size() == 0)
    {
        for (size_t i = 0; i < MAXCHRS; ++i)
        {
            playerNames[i] = party[i].playerName;
        }
    }
    return playerNames[playerId];
}

string KDraw::substitutePlayerNameString(const string& the_string)
{
    static const string pidxDelimiter("$");
    string replacedString(the_string);
    for (size_t i = 0; i < MAXCHRS; ++i)
    {
        string escapedPidx = pidxDelimiter + std::to_string(i);
        replacedString = replaceAll(replacedString, escapedPidx, getPlayerName(pidx[i]));
    }
    return replacedString.c_str();
}

const char* KDraw::decode_utf8(const char* inputString, uint32_t* cp)
{
    char ch = *inputString;

    if ((ch & 0b10000000) == 0x00)
    {
        /* single byte */
        *cp = (int)ch;
        ++inputString;
    }
    else if ((ch & 0b11100000) == 0b11000000)
    {
        /* double byte */
        *cp = ((ch & 0b00011111) << 6);
        ++inputString;
        ch = *inputString;

        if ((ch & 0b11000000) == 0b10000000)
        {
            *cp |= (ch & 0b00111111);
            ++inputString;
        }
        else
        {
            inputString = nullptr;
        }
    }
    else if ((ch & 0b11110000) == 0b11100000)
    {
        /* triple */
        *cp = (ch & 0b00001111) << 12;
        ++inputString;
        ch = *inputString;
        if ((ch & 0b11000000) == 0b10000000)
        {
            *cp |= (ch & 0b00111111) << 6;
            ++inputString;
            ch = *inputString;
            if ((ch & 0b11000000) == 0b10000000)
            {
                *cp |= (ch & 0b00111111);
                ++inputString;
            }
            else
            {
                inputString = nullptr;
            }
        }
        else
        {
            inputString = nullptr;
        }
    }
    else if ((ch & 0b11111000) == 0b11100000)
    {
        /* Quadruple */
        *cp = (ch & 0b00001111) << 18;
        ++inputString;
        ch = *inputString;
        if ((ch & 0b11000000) == 0b10000000)
        {
            *cp |= (ch & 0b00111111) << 12;
            ++inputString;
            ch = *inputString;
            if ((ch & 0b11000000) == 0b10000000)
            {
                *cp |= (ch & 0b00111111) << 6;
                ++inputString;
                ch = *inputString;
                if ((ch & 0b11000000) == 0b10000000)
                {
                    *cp |= (ch & 0b00111111);
                    ++inputString;
                }
                else
                {
                    inputString = nullptr;
                }
            }
            else
            {
                inputString = nullptr;
            }
        }
        else
        {
            inputString = nullptr;
        }
    }
    else
    {
        inputString = nullptr;
    }

    if (inputString == nullptr)
    {
        Game.program_death(_("UTF-8 decode error"));
    }
    return inputString;
}

int KDraw::get_glyph_index(uint32_t cp)
{
    if (cp < 128)
    {
        return cp - 32;
    }

    /* otherwise look up */
    auto glyph = KqFork::glyphLookup(cp);
    if (glyph != 0)
    {
        return glyph;
    }

    /* didn't find it */
    string glyphStr = "Invalid glyph index: " + std::to_string(cp);
    Game.klog(_(glyphStr.c_str()));
    return 0;
}

void KDraw::print_font(Raster* where, int sx, int sy, const char* msg, eFontColor font_index)
{
    int z = 0;
    int hgt = 8;//MagicNumber: font height for NORMAL text
    uint32_t cc = 0;

    if (font_index < 0 || font_index >= eFontColor::NUM_FONT_COLORS)
    {
        string errorStr = "print_font: Bad font index, " + std::to_string(static_cast<int>(font_index));
        Game.klog(_(errorStr.c_str()));
        return;
    }
    if (font_index == eFontColor::FONTCOLOR_BIG)
    {
        hgt = 12;//MagicNumber: font height for BIG text
    }
    while (1)
    {
        msg = decode_utf8(msg, &cc);
        if (cc == 0)
        {
            break;
        }
        cc = get_glyph_index(cc);
        masked_blit(kfonts, where, cc * 8, font_index * 8, z + sx, sy, 8, hgt);
        z += 8;
    }
}

void KDraw::print_num(Raster* where, int sx, int sy, const string& msg, eFont font_index)
{
    assert(where && "where == NULL");
    // Check ought not to be necessary if using the enum correctly.
    if (font_index >= eFont::NUM_FONTS)
    {
        string errorStr = "print_num: Bad font index, " + std::to_string(static_cast<int>(font_index));
        Game.klog(_(errorStr.c_str()));
        return;
    }
    for (size_t z = 0; z < msg.length(); z++)
    {
        // Convert each character in the string into a digit between 0..9
        auto cc = msg[z] - '0';
        if (cc >= 0 && cc <= 9)
        {
            masked_blit(sfonts[font_index], where, cc * 6, 0, z * 6 + sx, sy, 6, 8);
        }
    }
}

int KDraw::prompt(int who, int numopt, eBubbleStyle bstyle, const char* sp1, const char* sp2, const char* sp3, const char* sp4)
{
    int ptr = 0;

    setGbbhw(0, 1);
    setGbbs(0);
    drawGlobals::messageBuffer[0] = substitutePlayerNameString(sp1);
    drawGlobals::messageBuffer[1] = substitutePlayerNameString(sp2);
    drawGlobals::messageBuffer[2] = substitutePlayerNameString(sp3);
    drawGlobals::messageBuffer[3] = substitutePlayerNameString(sp4);
    Game.unpress();
    for (size_t a = 0; a < 4; a++)
    {
        const size_t str_len = drawGlobals::messageBuffer[a].length();
        if (str_len > 1)
        {
            setGbbh(a + 1);
            if ((signed int)str_len > getGbbw())
            {
                setGbbw(str_len);
            }
        }
    }

    set_textpos(who);
    if (getGbbw() == -1 || getGbbh() == -1)
    {
        return -1;
    }

    const int ly = (getGbbh() - numopt) * 12 + getGbby() + 10;
    while (true)
    {
        Game.do_check_animation();
        drawmap();
        draw_textbox(bstyle);

        draw_sprite(double_buffer, menuptr, getGbbx() + xofs + 8, ptr * 12 + ly + yofs);
        blit2screen(xofs, yofs);

        PlayerInput.readcontrols();
        if (PlayerInput.up)
        {
            Game.unpress();
            ptr = std::max(0, ptr - 1);
            play_effect(SND_CLICK, 128);
        }

        if (PlayerInput.down)
        {
            Game.unpress();
            ptr = std::min(numopt - 1, ptr + 1);
            play_effect(SND_CLICK, 128);
        }

        if (PlayerInput.balt)
        {
            Game.unpress();
            break;
        }
    }

    return ptr;
}

uint32_t KDraw::prompt_ex(uint32_t who, const char* ptext, const char* opt[], uint32_t n_opt)
{
    uint32_t curopt = 0;
    uint32_t topopt = 0;
    uint32_t winheight = 0;
    uint32_t winwidth = 0;
    uint32_t winx = 0;
    uint32_t winy = 0;
    bool running = false;

    ptext = substitutePlayerNameString(ptext).c_str();
    while (1)
    {
        setGbbw(1);
        setGbbs(0);
        ptext = splitTextOverMultipleLines(ptext).c_str();
        if (ptext)
        {
            /* print prompt pages prior to the last one */
            generic_text(who, eBubbleStyle::BUBBLE_TEXT, 0);
        }
        else
        {
            /* do prompt and options */

            /* calc the size of the prompt box */
            for (size_t a = 0; a < 4; a++)
            {
                size_t len = drawGlobals::messageBuffer[a].length();

                /* FIXME: PH changed >1 to >0 */
                if (len > 0)
                {
                    setGbbh(a + 1);
                    if ((signed int)len > getGbbw())
                    {
                        setGbbw(len);
                    }
                }
            }
            /* calc the size of the options box */
            for (size_t i = 0; i < n_opt; ++i)
            {
                while (isspace(*opt[i]))
                {
                    ++opt[i];
                }
                size_t w = strlen(opt[i]);
                if (winwidth < w)
                {
                    winwidth = w;
                }
            }
            winheight = n_opt > 4 ? 4 : n_opt;
            winx = xofs + (KQ_SCREEN_W - winwidth * 8) / 2;
            winy = yofs + (KQ_SCREEN_H - 10) - winheight * 12;
            running = true;
            while (running)
            {
                Game.do_check_animation();
                drawmap();
                /* Draw the prompt text */
                set_textpos(who);

                draw_textbox(eBubbleStyle::BUBBLE_TEXT);
                /* Draw the  options text */
                draw_kq_box(double_buffer, winx - 5, winy - 5, winx + winwidth * 8 + 13, winy + winheight * 12 + 5, eMenuBoxColor::SEMI_TRANSPARENT_BLUE, eBubbleStyle::BUBBLE_TEXT);
                for (size_t i = 0; i < winheight; ++i)
                {
                    print_font(double_buffer, winx + 8, winy + i * 12, opt[i + topopt], eFontColor::FONTCOLOR_BIG);
                }
                draw_sprite(double_buffer, menuptr, winx + 8 - menuptr->width, (curopt - topopt) * 12 + winy + 4);
                /* Draw the 'up' and 'down' markers if there are more options than will
                 * fit in the window */
                if (topopt > 0)
                {
                    draw_sprite(double_buffer, upptr, winx, winy - 8);
                }
                if (topopt < n_opt - winheight)
                {
                    draw_sprite(double_buffer, dnptr, winx, winy + 12 * winheight);
                }

                blit2screen(xofs, yofs);

                PlayerInput.readcontrols();
                if (PlayerInput.up && curopt > 0)
                {
                    play_effect(SND_CLICK, 128);
                    Game.unpress();
                    --curopt;
                }
                else if (PlayerInput.down && curopt < (n_opt - 1))
                {
                    play_effect(SND_CLICK, 128);
                    Game.unpress();
                    ++curopt;
                }
                else if (PlayerInput.balt)
                {
                    /* Selected an option */
                    play_effect(SND_CLICK, 128);
                    Game.unpress();
                    running = false;
                }
                else if (PlayerInput.bctrl)
                {
                    /* Just go "ow!" */
                    Game.unpress();
                    play_effect(SND_BAD, 128);
                }

                /* Adjust top position so that the current option is always shown */
                if (curopt < topopt)
                {
                    topopt = curopt;
                }
                if (curopt >= topopt + winheight)
                {
                    topopt = curopt - winheight + 1;
                }
            }
            return curopt;
        }
    }
}

string word_wrap(string text, size_t per_line)
{
    size_t line_begin = 0;

    while (line_begin < text.size())
    {
        const size_t ideal_end = line_begin + per_line;
        size_t line_end = ideal_end <= text.size() ? ideal_end : text.size() - 1;

        if (line_end == text.size() - 1)
        {
            ++line_end;
        }
        else if (std::isspace(text[line_end]))
        {
            text[line_end] = '\n';
            ++line_end;
        }
        else    // backtrack
        {
            unsigned end = line_end;
            while (end > line_begin && !std::isspace(text[end]))
            {
                --end;
            }

            if (end != line_begin)
            {
                line_end = end;
                text[line_end++] = '\n';
            }
            else
            {
                text.insert(line_end++, 1, '\n');
            }
        }

        line_begin = line_end;
    }

    return text;
}

/**
 * Given "longtext1 longtext2 longtext3 longtext4 longtext5 longtext6"
 * split into up to MSG_ROWS lines of text:
 *     "longtext1"
 *     "longtext2"
 *     "longtext3"
 *     "longtext4"
 * with all remaining text returned to the caller:
 *     "longtext5 longtext6"
 * for it to be processed later.
 */
string KDraw::splitTextOverMultipleLines(const string& stringToSplit)
{
    string wordWrappedString = word_wrap(stringToSplit, drawGlobals::MSG_COLS);

    for (size_t currentRow = 0; currentRow < drawGlobals::MSG_ROWS; ++currentRow)
    {
        auto singleRowNewlineAt = wordWrappedString.find_first_of('\n');
        string substring = wordWrappedString.substr(0, singleRowNewlineAt);
        if (singleRowNewlineAt == string::npos)
        {
            // No newlines, meaning nothing wrapped; everything fit on a single line
            drawGlobals::messageBuffer[currentRow++] = wordWrappedString;
            return "";
        }
    }

    int lasts, lastc, i, cr, cc;
    char tc;
    KqFork::eTextFormat state;

    for (i = 0; i < drawGlobals::MSG_ROWS; ++i)
    {
        drawGlobals::messageBuffer[i].clear();
    }
    i = 0;
    cc = 0;
    cr = 0;
    lasts = -1;
    lastc = 0;
    state = KqFork::eTextFormat::M_UNDEF;
    while (1)
    {
        tc = stringToSplit[i];
        switch (state)
        {
        case KqFork::eTextFormat::M_UNDEF:
            switch (tc)
            {
            case ' ':
                lasts = i;
                lastc = cc;
                state = KqFork::eTextFormat::M_SPACE;
                break;

            case '\0':
                //strbuf
                drawGlobals::messageBuffer[cr][cc] = '\0';
                state = KqFork::eTextFormat::M_END;
                break;

            case '\n':
                //strbuf
                drawGlobals::messageBuffer[cr][cc] = '\0';
                cc = 0;
                ++i;
                if (++cr >= 4)
                {
                    return &stringToSplit[i];
                }
                break;

            default:
                state = KqFork::eTextFormat::M_NONSPACE;
                break;
            }
            break;

        case KqFork::eTextFormat::M_SPACE:
            switch (tc)
            {
            case ' ':
                if (cc < drawGlobals::MSG_COLS - 1)
                {
                    //strbuf
                    drawGlobals::messageBuffer[cr][cc] = tc;
                    ++cc;
                }
                else
                {
                    drawGlobals::msgbuf[cr][drawGlobals::MSG_COLS - 1] = '\0';
                }
                ++i;
                break;

            default:
                state = KqFork::eTextFormat::M_UNDEF;
                break;
            }
            break;

        case KqFork::eTextFormat::M_NONSPACE:
            switch (tc)
            {
            case ' ':
            case '\0':
            case '\n':
                state = KqFork::eTextFormat::M_UNDEF;
                break;

            default:
                if (cc < drawGlobals::MSG_COLS - 1)
                {
                    drawGlobals::msgbuf[cr][cc++] = tc;
                }
                else
                {
                    drawGlobals::msgbuf[cr++][lastc] = '\0';
                    cc = 0;
                    i = lasts;
                    if (cr >= drawGlobals::MSG_ROWS)
                    {
                        return &stringToSplit[1 + lasts];
                    }
                }
                ++i;
                break;
            }
            break;

        case KqFork::eTextFormat::M_END:
            return nullptr;
            break;

        default:
            break;
        }
    }
}

void KDraw::revert_cframes(size_t fighter_index, int revert_heroes)
{
    size_t start_fighter_index, end_fighter_index;
    size_t cframe_index;

    /* Determine the range of frames to revert */
    if (revert_heroes == 1)
    {
        if (fighter_index < MAX_PARTY_SIZE)
        {
            start_fighter_index = 0;
            end_fighter_index = numchrs;
        }
        else
        {
            start_fighter_index = MAX_PARTY_SIZE;
            end_fighter_index = MAX_PARTY_SIZE + kqCombat.num_enemies;
        }
    }
    else
    {
        start_fighter_index = fighter_index;
        end_fighter_index = fighter_index + 1;
    }

    while (start_fighter_index < end_fighter_index)
    {
        for (cframe_index = 0; cframe_index < MAXCFRAMES; cframe_index++)
        {
            blit(tcframes[start_fighter_index][cframe_index], cframes[start_fighter_index][cframe_index], 0, 0, 0, 0, fighter[start_fighter_index].fighterImageDatafileWidth, fighter[start_fighter_index].fighterImageDatafileHeight);
        }
        ++start_fighter_index;
    }
}

void KDraw::set_textpos(uint32_t entity_index)
{
    if (entity_index < MAX_ENTITIES)
    {
        setGbxy((g_ent[entity_index].tilex * TILE_W) - camera_viewport_x,
                (g_ent[entity_index].tiley * TILE_H) - camera_viewport_y);
        setGbbx(getGbx() - (getGbbw() * 4));
        if (getGbbx() < 8)
        {
            setGbbx(8);
        }
        if (getGbbw() * 8 + getGbbx() + 16 > 312)
        {
            setGbbx(296 - (getGbbw() * 8));
        }
        if (getGby() > -16 && getGby() < KQ_SCREEN_H)
        {
            const int gbbh = getGbbh() * 12;
            if (g_ent[entity_index].facing == 1 || g_ent[entity_index].facing == 2)
            {
                if (gbbh + getGby() + 40 <= KQ_SCREEN_H - 8)
                {
                    setGbby(getGby() + 24);
                }
                else
                {
                    setGbby(getGby() - gbbh - 24);
                }
            }
            else
            {
                const int gby = getGby() - gbbh - 24;
                if (gby >= 8)
                {
                    setGbby(gby);
                }
                else
                {
                    setGbby(getGby() + 24);
                }
            }
        }
        else
        {
            if (getGby() < 8)
            {
                setGbby(8);
            }

            const int gbbh = getGbbh() * 12;
            if (gbbh + getGby() + 16 > 232)
            {
                setGbby(drawGlobals::magic_number_216 - gbbh);
            }
        }

        if (getGbby() > getGby())
        {
            setGby(getGby() + 20);
            setBubbleStemStyle(getGbx() < drawGlobals::magic_number_152 ? eBubbleStemStyle::STEM_TOP_LEFT : eBubbleStemStyle::STEM_TOP_RIGHT);
        }
        else
        {
            setGby(getGby() - 20);
            setBubbleStemStyle(getGbx() < drawGlobals::magic_number_152 ? eBubbleStemStyle::STEM_BOTTOM_LEFT : eBubbleStemStyle::STEM_BOTTOM_RIGHT);
        }
        if (getGbx() < getGbbx() + 8)
        {
            setGbx(getGbbx() + 8);
        }
        if (getGbx() > getGbbw() * 8 + getGbbx() - 8)
        {
            setGbx(getGbbw() * 8 + getGbbx() - 8);
        }
        if (getGby() < getGbby() - 4)
        {
            setGby(getGbby() - 4);
        }
        if (getGby() > getGbbh() * 12 + getGbby() + 4)
        {
            setGby(getGbbh() * 12 + getGbby() + 4);
        }
    }
    else
    {
        setGbbxy(drawGlobals::magic_number_152 - getGbbw() * 4,
                 drawGlobals::magic_number_216 - getGbbh() * 12);
        setBubbleStemStyle(eBubbleStemStyle::STEM_UNDEFINED);
    }
}

void KDraw::set_view(int vw, int x1, int y1, int x2, int y2)
{
    view_on = vw;
    if (view_on)
    {
        view_x1 = x1;
        view_y1 = y1;
        view_x2 = x2;
        view_y2 = y2;
    }
    else
    {
        view_x1 = 0;
        view_y1 = 0;
        view_x2 = g_map.xsize - 1;
        view_y2 = g_map.ysize - 1;
    }
}

void KDraw::text_ex(eBubbleStyle fmt, int who, const string& textToDisplay)
{
    auto s = substitutePlayerNameString(textToDisplay);

    while (s.length() > 0)
    {
        s = splitTextOverMultipleLines(s);
        generic_text(who, fmt, 0);
    }
}

void KDraw::porttext_ex(eBubbleStyle fmt, int who, const string& textToDisplay)
{
    auto s = substitutePlayerNameString(textToDisplay);

    while (s.length() > 0)
    {
        s = splitTextOverMultipleLines(s);
        generic_text(who, fmt, 1);
    }
}

KDraw kqDraw;
