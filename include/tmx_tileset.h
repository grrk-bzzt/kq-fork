#pragma once

#include <string>
#include <vector>

class KTmxTileset
{
public:
    KTmxTileset()
        : imagedata(nullptr)
    {}

    uint32_t firstgid;
    std::string name;
    std::string sourceimage;
    Raster* imagedata;
    std::vector<KTmxAnimation> animations;
    int width;
    int height;
};
