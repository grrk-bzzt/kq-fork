/*! \file
* \brief Bounding areas
* \author TT
* \date 20060720
*/

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "bounds.h"

bool KBounds::Add(std::shared_ptr<KBound> bound)
{
    m_bounds.push_back(bound);
    return true;
}

std::shared_ptr<KBound> KBounds::GetBound(size_t index)
{
    if (index < m_bounds.size())
    {
        return m_bounds[index];
    }
    return nullptr;
}

/*! \brief Determine whether given coordinates are within any bounding boxes
*
* \param   left - Left edge of current bounding area
* \param   top - Top edge of current bounding area
* \param   right - Right edge of current bounding area
* \param   bottom - Bottom edge of current bounding area
*
* \returns index+1 in array if found, else 0 if not found
*/
uint32_t KBounds::IsBound(const uint16_t left, const uint16_t top, const uint16_t right, const uint16_t bottom) const
{
    size_t i;
    uint16_t x1, y1, x2, y2;

    if (left <= right)
    {
        x1 = left;
        x2 = right;
    }
    else
    {
        x1 = right;
        x2 = left;
    }

    if (top <= bottom)
    {
        y1 = top;
        y2 = bottom;
    }
    else
    {
        y1 = bottom;
        y2 = top;
    }

    for (i = 0; i < m_bounds.size(); ++i)
    {
        auto current_bound = m_bounds[i];
        if (x1 > current_bound->right || x2 < current_bound->left || y1 > current_bound->bottom || y2 < current_bound->top)
        {
            continue;
        }
        else
        {
            return i + 1;
        }
    }

    return 0; // not found
}
