#pragma once

#include <memory>
#include <string>
#include <tinyxml2.h>
#include <vector>
using namespace tinyxml2;

#include "bounds.h"
#include "entity.h"
#include "markers.h"
#include "structs.h"
#include "tmx_animation.h"
#include "tmx_tileset.h"
#include "zone.h"

class tmx_layer
{
public:
    tmx_layer(int w, int h)
        : width(w)
        , height(h)
        , size(w * h)
        , data(new uint32_t[size])
    {
    }
    std::string name;
    const int width;
    const int height;
    const int size;
    std::unique_ptr<uint32_t[]> data;
};

class tmx_map
{
public:
    tmx_map();
    std::string name;
    int map_no;

    // Non-zero if zone 0 triggers an event
    bool zero_zone;

    // Map's parallax mode (see drawmap())
    int map_mode;

    // Non-zero if Save is allowed in this map
    bool can_save;

    // Which tile-set to use
    int tileset;

    // Non-zero if sunstone works on this map
    bool use_sstone;

    // Non-zero if Warp is allowed in this map
    bool can_warp;

    // Map width
    int xsize;

    // Map height
    int ysize;

    // Parallax multiplier
    int pmult;

    // Parallax divider
    int pdiv;

    // Default start x-coord
    int stx;

    // Default start y-coord
    int sty;

    // x-coord where warp spell takes you to (see special_spells())
    int warpx;

    // y-coord where warp spell takes you to (see special_spells())
    int warpy;

    // Internal revision number for the map file
    int revision;

    // Base file name for map song
    std::string song_file;

    // Map name (shown when map first appears)
    std::string description;

    // The name of the primary tileset (the one with gid=1)
    std::string primary_tileset_name;

    // Tilesets defined within this tilemap
    std::vector<KTmxTileset> tilesets;

    KBounds bounds;
    std::vector<KZone> zones;
    KMarkers markers;
    std::vector<KQEntity> entities;
    std::vector<tmx_layer> layers;
    void set_current();
    const KTmxTileset& find_tileset(const std::string&) const;
};

class KTiledMap
{
public:
    void load_tmx(const std::string&);

private:
    tmx_map load_tmx_map(XMLElement const* root);
    XMLElement const* find_tmx_element(XMLElement const*, const char*, const char*);
    KBounds load_tmx_bounds(XMLElement const*);
    KMarkers load_tmx_markers(XMLElement const*);
    std::vector<KZone> load_tmx_zones(XMLElement const*);
    tmx_layer load_tmx_layer(XMLElement const* el);
    std::vector<KQEntity> load_tmx_entities(XMLElement const*);
    KTmxTileset load_tmx_tileset(XMLElement const*);
    XMLElement const* find_objectgroup(XMLElement const* root, const char* name);
    std::vector<uint8_t> b64decode(const char*);
    std::vector<uint8_t> uncompress(const std::vector<uint8_t>& data);
};

extern KTiledMap TiledMap;
