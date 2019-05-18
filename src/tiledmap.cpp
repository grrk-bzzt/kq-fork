#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <tinyxml2.h>
#include <vector>
#define ZLIB_CONST
#include <zlib.h>
#include "animation.h"
#include "draw.h"
#include "enums.h"
#include "imgcache.h"
#include "kq.h"
#include "platform.h"
#include "structs.h"
#include "tiledmap.h"

using namespace tinyxml2;
KTiledMap TiledMap;

// Compatibility as VC insists we use these for safety
#ifdef _MSC_VER
using stdext::make_checked_array_iterator;
#else
template <typename T>
T* make_checked_array_iterator(T* ptr, size_t size, size_t offset = 0)
{
    (void)size;
    return ptr + offset;
}
#endif

struct s_zone
{
    int x;
    int y;
    int w;
    int h;
    int n;
};

struct tmx_tileset
{
    uint32_t firstgid;
    std::string name;
    std::string sourceimage;
    Raster* imagedata;
    std::vector<KTmxAnimation> animations;
    int width;
    int height;
};

// Ranged-for support, such as in load_tmx_layer(): "for (auto& tile : layer) {...}"
uint32_t* begin(tmx_layer& l)
{
    return l.data.get();
}
uint32_t* end(tmx_layer& l)
{
    return l.data.get() + l.size;
}

/** \brief Load a TMX format map from disk.
 * Make it the current map for the game
 * \param name the filename
 */
void KTiledMap::load_tmx(const std::string& name)
{
    XMLDocument tmx;
    std::string path = name + std::string(".tmx");
    tmx.LoadFile(kqres(MAP_DIR, path).c_str());
    if (tmx.Error())
    {
        TRACE("Error loading %s\n%s\n%s\n", name.c_str(), tmx.GetErrorStr1(), tmx.GetErrorStr2());
        Game.program_death("Could not load map file ");
    }
    Game.reset_timer_events();
    if (hold_fade == 0)
    {
        kqDraw.do_transition(TRANS_FADE_OUT, 4);
    }

    auto loaded_map = load_tmx_map(tmx.RootElement());
    loaded_map.set_current();
    Game.SetCurmap(name);
}

/// Convert a char* array to a string, where NULL returns an empty string.
static std::string strconv(const char* ptr)
{
    return std::string(ptr ? ptr : "");
}

tmx_map KTiledMap::load_tmx_map(XMLElement const* root)
{
    tmx_map smap;
    if (root == nullptr)
    {
        return smap;
    }

    // Map (of a town, dungeon) dimensions
    smap.xsize = root->IntAttribute("width");
    smap.ysize = root->IntAttribute("height");

    // Parallax
    smap.pmult = smap.pdiv = 1;

    // Markers
    smap.markers = load_tmx_markers(find_objectgroup(root, "markers"));

    // Bounding boxes
    smap.bounds = load_tmx_bounds(find_objectgroup(root, "bounds"));

    // Zones
    smap.zones = load_tmx_zones(find_objectgroup(root, "zones"));

    // Entities
    smap.entities = load_tmx_entities(find_objectgroup(root, "entities"));

    const auto properties = root->FirstChildElement("properties");
    if (properties == nullptr) {
        return smap;
    }

    constexpr char ELEM_PROPERTY[] = "property";
    for (auto xprop = properties->FirstChildElement(ELEM_PROPERTY); xprop != nullptr; xprop = xprop->NextSiblingElement(ELEM_PROPERTY))
    {
        auto value = xprop->FindAttribute("value");

        if (xprop->Attribute("name", "map_mode"))           {value->QueryIntValue(&smap.map_mode);}
        else if (xprop->Attribute("name", "map_no"))        {value->QueryIntValue(&smap.map_no);}
        else if (xprop->Attribute("name", "zero_zone"))     {value->QueryBoolValue(&smap.zero_zone);}
        else if (xprop->Attribute("name", "can_save"))      {value->QueryBoolValue(&smap.can_save);}
        else if (xprop->Attribute("name", "tileset"))       {value->QueryIntValue(&smap.tileset);}
        else if (xprop->Attribute("name", "use_sstone"))    {value->QueryBoolValue(&smap.use_sstone);}
        else if (xprop->Attribute("name", "can_warp"))      {value->QueryBoolValue(&smap.can_warp);}
        else if (xprop->Attribute("name", "pmult"))         {value->QueryIntValue(&smap.pmult);}
        else if (xprop->Attribute("name", "pdiv"))          {value->QueryIntValue(&smap.pdiv);}
        else if (xprop->Attribute("name", "stx"))           {value->QueryIntValue(&smap.stx);}
        else if (xprop->Attribute("name", "sty"))           {value->QueryIntValue(&smap.sty);}
        else if (xprop->Attribute("name", "warpx"))         {value->QueryIntValue(&smap.warpx);}
        else if (xprop->Attribute("name", "warpy"))         {value->QueryIntValue(&smap.warpy);}
        else if (xprop->Attribute("name", "song_file"))     {smap.song_file = strconv(value->Value());}
        else if (xprop->Attribute("name", "description"))   {smap.description = strconv(value->Value());}
    }

    // Tilesets
    for (auto xtileset = root->FirstChildElement("tileset"); xtileset != nullptr; xtileset = xtileset->NextSiblingElement("tileset"))
    {
        smap.tilesets.push_back(load_tmx_tileset(xtileset));
        // Make a note of the tileset with gid=1 for later
        const KTmxTileset& tileset = smap.tilesets.back();
        if (tileset.firstgid == 1)
        {
            smap.primary_tileset_name = tileset.name;
        }
    }

    // Load all the map layers (in order)
    constexpr char ELEM_LAYER[] = "layer";
    for (auto xlayer = root->FirstChildElement(ELEM_LAYER); xlayer != nullptr; xlayer = xlayer->NextSiblingElement(ELEM_LAYER))
    {
        smap.layers.push_back(load_tmx_layer(xlayer));
    }

    return smap;
}

/** \brief Load an array of bounding boxes from a TMX <objectgroup>.
 *
 * Note that tile-size of 16x16 is assumed here.
 * \param el the object group
 * \returns a collection of objects
 */
KBounds KTiledMap::load_tmx_bounds(XMLElement const* el)
{
    KBounds bounds;
    if (el)
    {
        for (auto i = el->FirstChildElement("object"); i != nullptr; i = i->NextSiblingElement("object"))
        {
            if (i->Attribute("type", "bounds"))
            {
                auto new_bound = std::make_shared<KBound>();
                new_bound->left = i->IntAttribute("x") / TILE_W;
                new_bound->top = i->IntAttribute("y") / TILE_H;
                new_bound->right = i->IntAttribute("width") / TILE_W + new_bound->left - 1;
                new_bound->bottom = i->IntAttribute("height") / TILE_H + new_bound->top - 1;
                new_bound->btile = 0;
                auto props = i->FirstChildElement("properties");
                if (props)
                {
                    constexpr char ELEM_PROPERTY[] = "property";
                    for (auto property = props->FirstChildElement(ELEM_PROPERTY); property != nullptr; property = property->NextSiblingElement(ELEM_PROPERTY))
                    {
                        if (property->Attribute("name", "btile"))
                        {
                            new_bound->btile = property->IntAttribute("value");
                        }
                    }
                }
                bounds.Add(new_bound);
            }
        }
    }
    return bounds;
}
/** \brief Scan tree for a named TMX <layer>.
 *
 * \param root the root of the tree
 * \param type the element tag
 * \param name the value of the 'name' attribute
 * \returns the found element or NULL
 */
XMLElement const* KTiledMap::find_tmx_element(XMLElement const* root, const char* type, const char* name)
{
    for (auto i = root->FirstChildElement(type); i != nullptr; i = i->NextSiblingElement(type))
    {
        if (i->Attribute("name", name))
        {
            return i;
        }
    }
    return nullptr;
}

/** \brief Load an array of markers from a TMX <objectgroup>.
 *
 * Note that tile-size of 16x16 is assumed here.
 * \param el the object group
 * \returns a collection of objects
 */
KMarkers KTiledMap::load_tmx_markers(XMLElement const* el)
{
    KMarkers markers;
    if (el)
    {
        for (auto obj = el->FirstChildElement("object"); obj != nullptr; obj = obj->NextSiblingElement("object"))
        {
            if (obj->Attribute("type", "marker"))
            {
                auto marker = std::make_shared<KMarker>();
                marker->x = obj->IntAttribute("x") / TILE_W;
                marker->y = obj->IntAttribute("y") / TILE_H;
                marker->markerName = obj->Attribute("name");
                markers.Add(marker);
            }
        }
    }
    return markers;
}

/** \brief Fetch tile indices from a layer.
 * The numbers are GIDs as stored in the TMX file.
 * \param el the layer element
 * \returns the raw data
 */
tmx_layer KTiledMap::load_tmx_layer(XMLElement const* el)
{
    auto h = el->IntAttribute("height");
    auto w = el->IntAttribute("width");
    tmx_layer layer(w, h);
    layer.name = el->Attribute("name");

    auto data = el->FirstChildElement("data");
    if (data == nullptr)
    {
        return layer;
    }

    if (data->Attribute("encoding", "csv"))
    {
        const char* raw = data->GetText();
        for (auto& tile : layer)
        {
            const char* next = strchr(raw, ',');
            tile = static_cast<uint32_t>(strtol(raw, nullptr, 10));

            if (next == nullptr)
            {
                break;
            }

            raw = next + 1;
        }
    }
    else if (data->Attribute("encoding", "base64"))
    {
        std::vector<uint8_t> bytes = b64decode(data->GetText());
        if (data->Attribute("compression", "zlib") == nullptr)
        {
            Game.program_death("Layer's compression not supported");
        }

        std::vector<uint8_t> raw = uncompress(bytes);
        if (raw.size() != layer.size * sizeof(uint32_t))
        {
            Game.program_death("Layer size mismatch");
        }

        auto iter = begin(raw);
        for (auto& tile : layer)
        {
            uint32_t v = *iter++;
            v |= (*iter++) << 8;
            v |= (*iter++) << 16;
            v |= (*iter++) << 24;
            tile = v;
        }
    }
    else
    {
        Game.program_death("Layer's encoding not supported");
    }

    return layer;
}
/** \brief Load up the zones
 * \param el the <objectgroup> element containing the zones
 * \returns a vector of zones
 */
std::vector<KZone> KTiledMap::load_tmx_zones(XMLElement const* el)
{
    std::vector<KZone> zones;
    if (el)
    {
        for (auto i = el->FirstChildElement("object"); i != nullptr; i = i->NextSiblingElement("object"))
        {
            if (i->Attribute("type", "zone"))
            {
                KZone zone;
                zone.x = i->IntAttribute("x") / TILE_W;
                zone.y = i->IntAttribute("y") / TILE_H;
                zone.w = i->IntAttribute("width") / TILE_W;
                zone.h = i->IntAttribute("height") / TILE_H;
                // TODO name might not always be an integer in future.
                zone.n = i->IntAttribute("name");
                zones.push_back(zone);
            }
        }
    }
    return zones;
}
/** \brief Load up the entities.
 * \param el the objectgroup element containing the entities
 * \returns a vector of entities
 */
std::vector<KQEntity> KTiledMap::load_tmx_entities(XMLElement const* el)
{
    std::vector<KQEntity> entities;
    for (auto i = el->FirstChildElement("object"); i != nullptr; i = i->NextSiblingElement("object"))
    {
        auto properties = i->FirstChildElement("properties");
        KQEntity entity;
        memset(&entity, 0, sizeof(entity));
        entity.x = i->IntAttribute("x");
        entity.y = i->IntAttribute("y");
        entity.tilex = entity.x / TILE_W;
        entity.tiley = entity.y / TILE_H;
        if (properties)
        {
            constexpr char ELEM_PROPERTY[] = "property";
            for (auto xprop = properties->FirstChildElement(ELEM_PROPERTY); xprop != nullptr; xprop = xprop->NextSiblingElement(ELEM_PROPERTY))
            {
                auto value = xprop->FindAttribute("value");
                if (xprop->Attribute("name", "chrx"))
                {
                    entity.setIdentity(value->IntValue());
                }
                if (xprop->Attribute("name", "eid"))
                {
                    entity.eid = value->IntValue();
                }
                if (xprop->Attribute("name", "active"))
                {
                    entity.active = (value->IntValue() != 0);
                }
                if (xprop->Attribute("name", "facing"))
                {
                    entity.facing = value->IntValue();
                }
                if (xprop->Attribute("name", "moving"))
                {
                    entity.moving = (value->IntValue() != 0);
                }
                if (xprop->Attribute("name", "framectr"))
                {
                    entity.framectr = value->IntValue();
                    if (entity.framectr >= MAX_FRAMECTR)
                    {
                        entity.framectr = MAX_FRAMECTR - 1;
                    }
                }
                if (xprop->Attribute("name", "movemode"))
                {
                    entity.movemode = value->IntValue();
                }
                if (xprop->Attribute("name", "obsmode"))
                {
                    entity.obsmode = value->IntValue();
                }
                if (xprop->Attribute("name", "delay"))
                {
                    entity.delay = value->IntValue();
                }
                if (xprop->Attribute("name", "delayctr"))
                {
                    entity.delayctr = value->IntValue();
                }
                if (xprop->Attribute("name", "speed"))
                {
                    entity.speed = value->IntValue();
                }
                if (xprop->Attribute("name", "scount"))
                {
                    entity.scount = value->IntValue();
                }
                if (xprop->Attribute("name", "cmd"))
                {
                    entity.cmd = value->IntValue();
                }
                if (xprop->Attribute("name", "sidx"))
                {
                    entity.sidx = value->IntValue();
                }
                if (xprop->Attribute("name", "chasing"))
                {
                    entity.chasing = value->IntValue();
                }
                if (xprop->Attribute("name", "cmdnum"))
                {
                    entity.cmdnum = value->IntValue();
                }
                if (xprop->Attribute("name", "atype"))
                {
                    entity.atype = value->IntValue();
                }
                if (xprop->Attribute("name", "snapback"))
                {
                    entity.snapback = value->IntValue();
                }
                if (xprop->Attribute("name", "facehero"))
                {
                    entity.facehero = value->IntValue();
                }
                if (xprop->Attribute("name", "transl"))
                {
                    entity.isSemiTransparent = (value->IntValue() != 0);
                }
                if (xprop->Attribute("name", "script"))
                {
                    strncpy(entity.script, value->Value(), sizeof(entity.script));
                }
            }
        }
        entities.push_back(entity);
    }
    return entities;
}

/** \brief Load a tileset.
 * This can be from a standalone file or embedded in a map.
 * \param el the <tileset> element
 * \returns the tileset
 */
KTmxTileset KTiledMap::load_tmx_tileset(XMLElement const* el)
{
    KTmxTileset tileset;
    tileset.firstgid = el->IntAttribute("firstgid");
    XMLElement const* tsx = nullptr;
    XMLDocument sourcedoc;
    auto source = el->Attribute("source");
    if (source)
    {
        // Specified 'source' so it's an external tileset. Load it.
        sourcedoc.LoadFile(kqres(MAP_DIR, source).c_str());
        if (sourcedoc.Error())
        {
            TRACE("Error loading %s\n%s\n%s\n", source, sourcedoc.GetErrorStr1(), sourcedoc.GetErrorStr2());
            Game.program_death("Couldn't load external tileset");
        }
        tsx = sourcedoc.RootElement();
    }
    else
    {
        // No 'source' so it's internal; use the element itself
        tsx = el;
    }

    auto name = tsx->Attribute("name");
    if (name)
    {
        tileset.name = name;
    }
    // Get the image
    XMLElement const* image = tsx->FirstChildElement("image");
    tileset.sourceimage = image->Attribute("source");
    tileset.width = image->IntAttribute("width");
    tileset.height = image->IntAttribute("height");
    tileset.imagedata = get_cached_image(tileset.sourceimage);

    // Get the animation data from the .tsx file
    for (auto xtile = tsx->FirstChildElement("tile"); xtile != nullptr; xtile = xtile->NextSiblingElement("tile"))
    {
        KTmxAnimation anim;
        anim.setTileNumber(xtile->IntAttribute("id"));
        auto xanim = xtile->FirstChildElement("animation");
        if (xanim)
        {
            for (auto xframe = xanim->FirstChildElement("frame"); xframe != nullptr; xframe = xframe->NextSiblingElement("frame"))
            {
                KTmxAnimation::animation_frame frame;
                frame.delay = xframe->IntAttribute("duration");
                frame.tile = xframe->IntAttribute("tileid");
                anim.frames.push_back(frame);
            }
        }
        tileset.animations.push_back(anim);
    }

    return tileset;
}

XMLElement const* KTiledMap::find_objectgroup(XMLElement const* root, const char* name)
{
    return find_tmx_element(root, "objectgroup", name);
}

tmx_map::tmx_map()
    : map_no(0)
    , zero_zone(false)
    , map_mode(0)
    , can_save(false)
    , tileset(0)
    , use_sstone(false)
    , can_warp(false)
    , xsize(0)
    , ysize(0)
    , pmult(1)
    , pdiv(1)
    , stx(0)
    , sty(0)
    , warpx(0)
    , warpy(0)
    , revision(1)
{
}

static const uint16_t SHADOW_OFFSET = 200;
/*! \brief Make this map the current one.
 * Make this map the one in play by copying its information into the
 * global structures. This function is the 'bridge' between the
 * TMX loader and the original KQ code.
 */
void tmx_map::set_current()
{
    // general map properties
    g_map.xsize = xsize;
    g_map.ysize = ysize;
    g_map.map_no = map_no;
    g_map.can_save = can_save;
    g_map.can_warp = can_warp;
    g_map.pdiv = (pdiv == 0 ? 1 : pdiv);
    g_map.pmult = pmult;
    g_map.map_mode = map_mode;
    g_map.stx = stx;
    g_map.sty = sty;
    g_map.warpx = warpx;
    g_map.warpy = warpy;
    g_map.tileset = tileset;
    g_map.use_sstone = use_sstone;
    g_map.zero_zone = zero_zone;
    g_map.map_desc = description;
    g_map.song_file = song_file;
    // Markers
    g_map.markers = markers;
    // Bounding boxes
    g_map.bounds = bounds;
    // Allocate space for layers
    for (auto && layer : layers)
    {
        if (layer.name == "map")
        {
            // map layers - these always have tile offset == 1
            free(map_seg);
            unsigned short* ptr = map_seg = static_cast<unsigned short*>(calloc(layer.size, sizeof(*map_seg)));
            for (auto t : layer)
            {
                if (t > 0)
                {
                    --t;
                }
                *ptr++ = static_cast<unsigned short>(t);
            }
        }
        else if (layer.name == "bmap")
        {
            free(b_seg);
            unsigned short* ptr = b_seg = static_cast<unsigned short*>(calloc(layer.size, sizeof(*b_seg)));
            for (auto t : layer)
            {
                if (t > 0)
                {
                    --t;
                }
                *ptr++ = t;
            }
        }
        else if (layer.name == "fmap")
        {
            free(f_seg);
            unsigned short* ptr = f_seg = static_cast<unsigned short*>(calloc(layer.size, sizeof(*f_seg)));
            for (auto t : layer)
            {
                if (t > 0)
                {
                    --t;
                }
                *ptr++ = t;
            }
        }
        else if (layer.name == "shadows")
        {
            // Shadows
            unsigned short shadow_offset = find_tileset("misc").firstgid + SHADOW_OFFSET;
            free(s_seg);
            auto sptr = s_seg = static_cast<unsigned char*>(calloc(layer.size, sizeof(*s_seg)));
            for (auto t : layer)
            {
                if (t > 0)
                {
                    t -= shadow_offset;
                }
                *sptr++ = static_cast<unsigned char>(t);
            }
        }
        else if (layer.name == "obstacles")
        {
            // Obstacles
            unsigned short obstacle_offset = find_tileset("obstacles").firstgid - 1;
            free(o_seg);
            auto sptr = o_seg = static_cast<unsigned char*>(calloc(layer.size, sizeof(*o_seg)));

            for (auto t : layer)
            {
                if (t > 0)
                {
                    t -= obstacle_offset;
                }
                *sptr++ = static_cast<unsigned char>(t);
            }
        }
    }

    // Zones
    free(z_seg);
    z_seg = static_cast<unsigned char*>(calloc(xsize * ysize, sizeof(unsigned char)));
    for (auto && zone : zones)
    {
        for (int i = 0; i < zone.w; ++i)
        {
            for (int j = 0; j < zone.h; ++j)
            {
                z_seg[(i + zone.x) + xsize * (j + zone.y)] = zone.n;
            }
        }
    }

    // Entities
    memset(&allEntitiesOnTheMap[MAX_PARTY_SIZE], 0, (MAX_ENTITIES - MAX_PARTY_SIZE) * sizeof(KQEntity));
    copy(begin(entities), end(entities), make_checked_array_iterator(allEntitiesOnTheMap, MAX_ENTITIES, MAX_PARTY_SIZE));

    // Tilemaps
    g_map.map_tiles = find_tileset(primary_tileset_name).imagedata;
    g_map.misc_tiles = find_tileset("misc").imagedata;
    g_map.entity_tiles = find_tileset("entities").imagedata;

    // Animations
    kqAnimation.clear_animations();
    for (auto& a : find_tileset(primary_tileset_name).animations)
    {
        kqAnimation.add_animation(a);
    }
}

const KTmxTileset& tmx_map::find_tileset(const std::string& name) const
{
    for (auto && ans : tilesets)
    {
        if (ans.name == name)
        { return ans; }
    }
    // not found
    TRACE("Tileset '%s' not found in map.\n", name.c_str());
    Game.program_death("No such tileset");
}

/*! \brief BASE64 scanner.
 * This iterates through some BASE64 characters, returning each 6-bit segment in turn.
 * If it gets an '=' character it returns PAD.
 * It ignores whitespace.
 * Any other characters return ERROR and set the error bit.
 * When it reaches the end of the string it returns PAD forever
 */
struct b64
{
    b64(const char* _ptr)
        : ptr(_ptr)
        , errbit(false)
    {
        while (isspace(*ptr))
        {
            ++ptr;
        }
    }

    uint8_t operator()()
    {
        if (ptr)
        {
            const char* pos = strchr(validchars, *ptr++);
            while (isspace(*ptr))
            {
                ++ptr;
            }
            if (*ptr == 0)
            {
                ptr = nullptr;
            }
            if (pos)
            {
                return static_cast<uint8_t>(pos - validchars);
            }
            else
            {
                errbit = true;
                ptr = nullptr;
                return ERROR;
            }
        }
        else
        {
            return PAD;
        }
    }

    bool error() const
    {
        return errbit;
    }

    void seterror()
    {
        errbit = true;
    }

    operator bool() const
    {
        return ptr != nullptr;
    }

    static const char* validchars;
    static const uint8_t PAD = 0x40;
    static const uint8_t ERROR = 0xff;
    const char* ptr;
    bool errbit;
};

const char* b64::validchars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                              "abcdefghijklmnopqrstuvwxyz"
                              "0123456789"
                              "+/=";

/// Return true if a byte is a valid 6-bit block.
static bool valid(uint8_t v)
{
    return v < b64::PAD;
}

/*! \brief Decode BASE64.
 * Convert a string of characters into a vector of bytes.
 * If there is an error, returns an empty vector.
 * \param text the input characters
 * \returns the converted bytes
 */
std::vector<uint8_t> KTiledMap::b64decode(const char* text)
{
    std::vector<uint8_t> data;
    b64 nextc(text);
    while (nextc)
    {
        uint8_t b0 = nextc();
        if (valid(b0))
        {
            uint8_t b1 = nextc();
            if (valid(b1))
            {
                data.push_back(b0 << 2 | b1 >> 4);
                uint8_t b2 = nextc();
                if (valid(b2))
                {
                    data.push_back(b1 << 4 | b2 >> 2);
                    uint8_t b3 = nextc();
                    if (valid(b3))
                    {
                        data.push_back(b2 << 6 | b3);
                    }
                }
                else
                {
                    //  if b2 is pad then b3 has to be too.
                    if (b2 == b64::PAD && nextc() != b64::PAD)
                    {
                        nextc.seterror();
                    }
                }
            }
            else
            {
                //  if b1 is pad this is an error
                nextc.seterror();
            }
        }
        else
        {
            // if b0 is pad this is an error
            nextc.seterror();
        }
    }
    if (!nextc.error())
    {
        return data;
    }
    else
    {
        return std::vector<uint8_t>();
    }
}

/*! \brief Uncompress a sequence of bytes.
 * Uses the zlib to uncompress.
 * \param data the input compressed data
 * \returns the uncompressed data
 */
std::vector<uint8_t> KTiledMap::uncompress(const std::vector<uint8_t>& data)
{
    z_stream stream;
    std::vector<uint8_t> out;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = data.size();
    stream.next_in = reinterpret_cast<z_const Bytef*>(data.data());

    if (inflateInit(&stream) == Z_OK)
    {
        uint8_t buffer[256];
        stream.avail_out = sizeof(buffer);
        stream.next_out = buffer;
        while (true)
        {
            int rc = inflate(&stream, Z_NO_FLUSH);
            if (rc < 0)
            {
                // Error
                out.clear();
                break;
            }
            if (stream.avail_out < sizeof(buffer))
            {
                std::copy(buffer, buffer + sizeof(buffer) - stream.avail_out, back_inserter(out));
                stream.avail_out = sizeof(buffer);
                stream.next_out = buffer;
            }
            if (rc == Z_STREAM_END)
            {
                break;
            };
        }
        inflateEnd(&stream);
    }
    return out;
}
