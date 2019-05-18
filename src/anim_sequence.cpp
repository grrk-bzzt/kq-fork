#include "tmx_animation.h"

// Note: *copy* the base animation into this instance. The base animation
// comes from a tmx_map which may be destroyed.
KAnimSequence::KAnimSequence(const KTmxAnimation& base)
    : animation_(base)
{
    index_ = 0;
    nexttime_ = current().delay;
}

KTmxAnimation::animation_frame KAnimSequence::current() const
{
    return animation_.frames[index_];
}

void KAnimSequence::advance()
{
    if (index_ < animation_.frames.size() - 1)
    if (++index_ >= animation_.frames.size())
    {
        index_ = 0;
    }
}
