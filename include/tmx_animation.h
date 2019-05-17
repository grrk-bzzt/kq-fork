#pragma once

#include <vector>

class KTmxAnimation
{
public:
    void setTileNumber(int mTileNumber);
    int getTileNumber() const;

	struct animation_frame
	{
		int tile;  //!< New tile value
		int delay; //!< Delay in milliseconds before showing this tile
	};

    std::vector<animation_frame> frames; //!< Sequence of animation frames

private:
    /**
     * Animated tiles start at this index
     */
    int baseTileNumber_; //!< Base tile number to be altered
};
