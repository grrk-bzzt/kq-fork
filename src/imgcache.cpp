#include "imgcache.h"
#include "gfx.h"
#include "kq.h"
#include "platform.h"
#include "res.h"
#include <map>
#include <memory>
#include <png.h>
#include <string>

typedef std::unique_ptr<Raster> BITMAP_PTR;

class image_cache
{
public:
    Raster* get(const std::string& name);
    void clear();

private:
    std::map<std::string, BITMAP_PTR> cache;
};

// At the moment there is one global cache; in the future multiple caches could be
// created and destroyed.
static image_cache global;

static int palindex(uint8_t* ptr)
{
    // Allegro's palettes are 0..63
    uint8_t r = ptr[0] >> 2;
    uint8_t g = ptr[1] >> 2;
    uint8_t b = ptr[2] >> 2;
    uint8_t a = ptr[3];

    // Any transparency at all means return the palette transparent color (0)
    if (a != 0xFF)
    {
        return 0;
    }
    int bestindex = 255, bestdist = 0x1000;
    // Start at 1 because 0 is the transparent color and we don't want to match
    // it
    for (int i = 1; i < 256; ++i)
    {
        RGB& rgb = pal[i];
        int dist = ABS(r - rgb.r) + ABS(g - rgb.g) + ABS(b - rgb.b);
        if (dist == 0)
        {
            // Exact match, early return
            return i;
        }
        else
        {
            if (dist < bestdist)
            {
                bestdist = dist;
                bestindex = i;
            }
        }
    }
    return bestindex;
}

// For libpng 1.6 and above there's a high-level image loader
#ifdef PNG_SIMPLIFIED_READ_SUPPORTED
/*! \brief Load a bitmap from a file
  * Allocate space for and load a bitmap in PNG format.
  * Assumed that we're running in 8bpp mode using KQ's palette.
  * Returns null if not found or error while loading
  * \param path the filename
  * \returns the bitmap
*/
static Raster* bmp_from_png(const std::string& path)
{
    png_image image;
    image.version = PNG_IMAGE_VERSION;
    image.opaque = nullptr;
    png_image_begin_read_from_file(&image, path.c_str());
    Raster* bitmap = nullptr;
    if (!PNG_IMAGE_FAILED(image))
    {
        // Force load in true color with alpha format
        image.format = PNG_FORMAT_RGBA;
        std::unique_ptr<uint8_t[]> imagedata(new uint8_t[PNG_IMAGE_SIZE(image)]);
        png_image_finish_read(&image, nullptr, imagedata.get(), PNG_IMAGE_ROW_STRIDE(image), nullptr);
        bitmap = new Raster(image.width, image.height);
        // Then convert to paletted.
        // This can be optimised or go away later
        for (auto y = 0u; y < image.height; ++y)
        {
            auto pptr = &imagedata[y * PNG_IMAGE_ROW_STRIDE(image)];
            for (auto x = 0u; x < image.width; ++x)
            {
                bitmap->setpixel(x, y, palindex(pptr));
                pptr += PNG_IMAGE_PIXEL_SIZE(PNG_FORMAT_RGBA);
            }
        }
    }
    png_image_free(&image);
    return bitmap;
}
#else // !PNG_SIMPLIFIED_READ_SUPPORTED
#include <cstdio>
static Raster* bmp_from_png(const std::string& path)
{
    FILE* fp = std::fopen(path.c_str(), "rb");
    if (!fp)
    {
        return nullptr;
    }
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png_ptr)
    { return nullptr; }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        std::fclose(fp);
        png_destroy_read_struct(&png_ptr, nullptr, nullptr);
        return nullptr;
    }
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        std::fclose(fp);
        return nullptr;
    }
    png_init_io(png_ptr, fp);
    png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_STRIP_16, nullptr);
    auto row_pointers = png_get_rows(png_ptr, info_ptr);
    auto width = png_get_image_width(png_ptr, info_ptr);
    auto height = png_get_image_height(png_ptr, info_ptr);
    Raster* bitmap = new Raster(width, height);
    // Then convert to paletted.
    // This can be optimised or go away later
    for (auto y = 0u; y < height; ++y)
    {
        auto pptr = row_pointers[y];
        for (auto x = 0u; x < width; ++x)
        {
            bitmap->ptr(x, y) = palindex(pptr);
            pptr += sizeof(uint32_t);
        }
    }
    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
    fclose(fp);
    return bitmap;
}
#endif

/*! \brief Get or load an image.
 * Return the image from the cache or load it.
 * The returned Raster is owned by the cache so do not delete it.
 * It's program_death if the image can't be loaded.
 * \param name the file base name
 * \returns the bitmap
*/
Raster* image_cache::get(const std::string& name)
{
    auto entry = cache.find(name);
    if (entry == cache.end())
    {
        // Not found, try to load
        Raster* bmp = bmp_from_png(kqres(DATA_DIR, name));
        if (!bmp)
        {
            // Try also in maps because it may be a tileset graphic
            bmp = bmp_from_png(kqres(MAP_DIR, name));
        }
        if (!bmp)
        {
            TRACE("Cannot load bitmap '%s'\n", name.c_str());
            Game.program_death("Error loading image.");
        }
        cache.insert(std::make_pair(name, BITMAP_PTR(bmp)));
        return bmp;
    }
    else
    {
        return entry->second.get();
    }
}

/*! \brief clear the image cache.
 * Remove all entries, delete the corresponding bitmaps
 */
void image_cache::clear()
{
    cache.clear();
}

/*! \brief get image from the global cache
 * \param name the name of the image file
 * \returns a bitmap
 */
Raster* get_cached_image(const std::string& name)
{
    return global.get(name);
}

/*! \brief clear the global cache. */
void clear_image_cache()
{
    global.clear();
}
