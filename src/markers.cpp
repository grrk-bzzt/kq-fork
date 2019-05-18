/*! \file
 * \brief Markers
 * \author OC
 * \date 20100222
 */

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <memory>

#include "markers.h"
KMarkers::KMarkers() {}

KMarkers::~KMarkers()
{
    for (auto& marker : m_markers)
    {
        marker = nullptr;
    }
    m_markers.clear();
}

bool KMarkers::Add(std::shared_ptr<KMarker> marker)
{
    m_markers.push_back(marker);
    return true;
}

bool KMarkers::Remove(std::shared_ptr<KMarker> marker)
{
    auto found = std::find(m_markers.begin(), m_markers.end(), marker);
    if (found != m_markers.end())
    {
        m_markers.erase(found);
        return true;
    }
    return false;
}

std::shared_ptr<KMarker> KMarkers::GetMarker(size_t index)
{
    if (index < m_markers.size())
    {
        return m_markers[index];
    }
    return nullptr;
}

std::shared_ptr<KMarker> KMarkers::GetMarker(const std::string& marker_name)
{
    for (auto& marker : m_markers)
    {
        if (marker->markerName == marker_name)
        {
            return marker;
        }
    }
    return nullptr;
}

std::shared_ptr<KMarker> KMarkers::GetMarker(int32_t x, int32_t y)
{
    for (auto& marker : m_markers)
    {
        if (marker->x == x && marker->y == y)
        {
            return marker;
        }
    }
    return nullptr;
}

KMarker Marker;
