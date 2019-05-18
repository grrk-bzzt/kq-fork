#pragma once

#include <memory>
#include <string>
#include <vector>

/*! \file
 * \brief Structures common to mapedit and the game (s_map and s_entity)
 * \author PH
 * \date 20030805
 */

/*! \brief Position marker
 *
 * A marker is a named position on the map.
 * These are set up in the mapeditor and accessed via scripts.
 * They are to make it easier to pinpoint locations in the maps,
 * for example, the locations that doors lead to.
 * \author PH
 * \date 20050126
 */
struct KMarker
{
    // The name of the current marker
    std::string markerName;

    // The X position the marker refers to
    int32_t x;

    // The Y position the marker refers to
    int32_t y;
};

class KMarkers
{
public:
    KMarkers();
    ~KMarkers();

    // Add a new marker to the map. Returns true on success, or false on failure.
    bool Add(std::shared_ptr<KMarker> marker);

    // Remove the specified marker from the map. Returns true if the marker was
    // removed, or false if the marker was not found.
    bool Remove(std::shared_ptr<KMarker> marker);

    // Return a pointer to the marker at the given @param index. If index is
    // invalid, returns null.
    std::shared_ptr<KMarker> GetMarker(size_t index);

    // Return a pointer to the marker that has the given @param name. If no
    // markers by that name are found, returns null.
    std::shared_ptr<KMarker> GetMarker(const std::string& name);

    // Return a pointer to the marker whose @param x and @param y coordinates
    // match. If no marker is at those coordinates, returns null.
    std::shared_ptr<KMarker> GetMarker(int32_t x, int32_t y);

    // Return the number of markers in the array.
    inline size_t Size() const { return m_markers.size(); }

protected:
    std::vector<std::shared_ptr<KMarker>> m_markers;
};

extern KMarker Marker;
