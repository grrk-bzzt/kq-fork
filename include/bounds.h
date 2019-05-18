#pragma once

#include <allegro.h>
#include <memory>
#include <vector>

struct PACKFILE;

/*! \file
 * \brief Contains functions specific to bounded areas
 * \author TT
 * \date 20060720
 */

/*! \brief Bounding area box
 *
 * A boundary is a viewable area on the map.
 * These are set up in the map editor to remove the need to call set_view
 * in the scripts. If the player is inside a bounding box' coordinate, then
 * the map will only redraw those tiles, else it will redraw everything on the
 * map.
 * \author TT
 * \date 20060710
 */
struct KBound
{
public:
    short left;   /*!< Left edge of the bounding box */
    short top;    /*!< Top edge of the bounding box */
    short right;  /*!< Right edge of the bounding box */
    short bottom; /*!< Bottom edge of the bounding box */
    short btile;  /*!< Index of the tile to draw everywhere BUT here */
};

/*! \brief Container holding array of bounds
 *
 * This contains an array of bounds, and the number of bounds, to simplify
 * passing around the size and elements separately.
 *
 * \author OC
 * \date 20101017
 */
class KBounds
{
public:
    KBounds()
    {
    }
    ~KBounds()
    {
    }

    // Add a new bound to the map. Returns true on success, or false on failure.
    bool Add(std::shared_ptr<KBound> bound);

    // Return a pointer to the bound at the given @param index. If index is
    // invalid, returns null.
    std::shared_ptr<KBound> GetBound(size_t index);

    size_t Size()
    {
        return m_bounds.size();
    }

    uint32_t IsBound(const uint16_t left, const uint16_t top, const uint16_t right, const uint16_t bottom) const;

protected:
    std::vector<std::shared_ptr<KBound>> m_bounds;
};
