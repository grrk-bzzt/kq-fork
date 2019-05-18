#pragma once

#include <stdint.h>

struct BITMAP;

class Raster
{
public:
    Raster(uint16_t w, uint16_t h);
    Raster(Raster&& other);
    ~Raster();
    void blitTo(Raster* target, int16_t src_x, int16_t src_y, uint16_t src_w, uint16_t src_h, int16_t dest_x, int16_t dest_y, uint16_t dest_w, uint16_t dest_h, bool masked);
    void blitTo(Raster* target, int16_t src_x, int16_t src_y, uint16_t dest_x, uint16_t dest_y, uint16_t src_w, uint16_t src_h, bool masked);
    void blitTo(Raster* target, int16_t src_x, int16_t src_y, int16_t dest_x, int16_t dest_y, uint16_t src_w, uint16_t src_h);
    void blitTo(Raster* target, int16_t dest_x, int16_t dest_y);
    void blitTo(Raster* target);
    void maskedBlitTo(Raster* target, int16_t dest_x, int16_t dest_y);
    void setpixel(int16_t x, int16_t y, uint8_t color);
    uint8_t getpixel(int16_t x, int16_t y);
    void hline(int16_t x0, int16_t x1, int16_t y, uint8_t color);
    void vline(int16_t x, int16_t y0, int16_t y1, uint8_t color);
    void fill(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t color);
    void fill(uint8_t color);
    uint8_t& ptr(int16_t x, int16_t y);
    const uint16_t width, height;
    const uint16_t stride;

private:
    uint8_t* data;
};

// Compatibility stuff
Raster* raster_from_bitmap(BITMAP*);
inline void blit(Raster* src, Raster* dest, int sx, int sy, int dx, int dy, int w, int h)
{
    src->blitTo(dest, sx, sy, dx, dy, w, h, false);
}
inline void masked_blit(Raster* src, Raster* dest, int sx, int sy, int dx, int dy, int w, int h)
{
    src->blitTo(dest, sx, sy, dx, dy, w, h, true);
}
inline void stretch_blit(Raster* src, Raster* dest, int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh)
{
    src->blitTo(dest, sx, sy, sw, sh, dx, dy, dw, dh, false);
}
inline void draw_sprite(Raster* dest, Raster* src, int x, int y)
{
    src->maskedBlitTo(dest, x, y);
}
inline void rectfill(Raster* dest, int x1, int y1, int x2, int y2, int c)
{
    dest->fill(x1, y1, x2 - x1, y2 - y1, c);
}
inline void vline(Raster* dest, uint16_t x, uint16_t y0, uint16_t y1, uint8_t color)
{
    dest->vline(x, y0, y1, color);
}
inline void hline(Raster* dest, uint16_t x0, uint16_t y, uint16_t x1, uint8_t color)
{
    dest->hline(x0, x1, y, color);
}
inline void clear_to_color(Raster* r, int c)
{
    r->fill(c);
}
inline void clear_bitmap(Raster* r)
{
    r->fill(0);
}
inline void putpixel(Raster* r, int x, int y, int c)
{
    r->setpixel(x, y, c);
}
void ellipsefill(Raster* r, int x, int y, int rx, int ry, int color);
inline void circlefill(Raster* r, int x, int y, int radius, int color)
{
    ellipsefill(r, x, y, radius, radius, color);
}
void draw_trans_sprite(Raster* dest, Raster* src, int x, int y);
inline void rect(Raster* r, int x1, int y1, int x2, int y2, int c)
{
    r->vline(x1, y1, y2, c);
    r->vline(x2, y1, y2, c);
    r->hline(x1, x2, y1, c);
    r->hline(x1, x2, y2, c);
}
