#include "tmx_animation.h"

void KTmxAnimation::setTileNumber(int mTileNumber)
{
    baseTileNumber_ = mTileNumber;
}

int KTmxAnimation::getTileNumber() const
{
    return baseTileNumber_;
}
