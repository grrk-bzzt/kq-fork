/*! \file
 * \brief NPC movement routines
 *
 * Set of functions to allow NPCs move to a certain spot without having
 * to tell the directions.
 *
 * \author RB
 * \date March 2005
 */

#include "entity.h"
#include "kq.h"
#include "movement.h"
#include <sstream>
#include <stdio.h>
#include <string>

KMovement kMovement;

namespace {

    constexpr int32_t kImpassibleBlock = -1;
    constexpr int32_t kNotYetProcessed = 0;

} // anonymous namespace

KMovement::KMovement()
{

}

KMovement::eStatus KMovement::find_path(std::string& out_buffer, size_t entity_id, uint32_t source_x, uint32_t source_y, uint32_t target_x, uint32_t target_y)
{
    // Allocate a 0-filled array for each tile within the map (kNotYetProcessed == 0).
    int32_t* map = (int32_t*)calloc(g_map.xsize * g_map.ysize, sizeof(int32_t));
    if (map == nullptr)
    {
        return eStatus::OUT_OF_MEMORY;
    }

    // Set all impassible tile values to kImpassibleBlock (-1).
    mark_obstacles_in_map(map);

    eStatus result = search_paths(map, entity_id, 1, source_x, source_y, target_x, target_y);

    if (result != eStatus::PATH_FOUND)
    {
        free(map);
        return result;
    }

    result = compose_path(out_buffer, map, target_x, target_y);

    free(map);
    return result;
}

KMovement::eStatus KMovement::compose_path(std::string& out_buffer, const int32_t* map, uint32_t target_x, uint32_t target_y)
{
    if (target_x >= g_map.xsize || target_y >= g_map.ysize || map == nullptr)
    {
        return eStatus::UNKNOWN_ERROR;
    }

    uint32_t x = target_x;
    uint32_t y = target_y;
    int32_t value = map[y * g_map.xsize + x];
    int index = value - 2;

    char temp[1024];
    memset(temp, '\0', sizeof(temp));

    while (value > 1)
    {
        /*  move as many squares up as possible  */
        while (y > 0 && value > 1 && map[(y - 1) * g_map.xsize + x] == value - 1)
        {
            value--;
            y--;
            temp[index--] = 'D';
        }

        /*  move as many squares left as possible  */
        while (x > 0 && value > 1 && map[y * g_map.xsize + (x - 1)] == value - 1)
        {
            value--;
            x--;
            temp[index--] = 'R';
        }

        /*  move as many squares down as possible  */
        while (y < g_map.ysize - 1 && value > 1 && map[(y + 1) * g_map.xsize + x] == value - 1)
        {
            value--;
            y++;
            temp[index--] = 'U';
        }

        /*  move as many squares right as possible  */
        while (x < g_map.xsize - 1 && value > 1 && map[y * g_map.xsize + (x + 1)] == value - 1)
        {
            value--;
            x++;
            temp[index--] = 'L';
        }
    }

    return (minimize_path(out_buffer, temp));
}

void KMovement::mark_obstacles_in_map(int32_t* map)
{
    if (map == nullptr)
    {
        return;
    }

    for (uint32_t y = 0; y < g_map.ysize; y++)
    {
        for (uint32_t x = 0; x < g_map.xsize; x++)
        {
            size_t index = y * g_map.xsize + x;

            if (o_seg[index] == eObstacle::BLOCK_ALL)
            {
                map[index] = kImpassibleBlock;
            }
        }
    }

    // RB: faster to do this than to check if there is an entity at every square
    for (auto& entity : g_ent)
    {
        if (entity.active && entity.tilex < g_map.xsize && entity.tiley < g_map.ysize)
        {
            map[entity.tiley * g_map.xsize + entity.tilex] = kImpassibleBlock;
        }
    }
}

bool KMovement::isValidCommand(const char input)
{
    // Note: All of these, except default, are implied "fall-through":
    switch (input)
    {
    default:
        return false;

    // eCommands::COMMAND_MOVE_UP
    case 'u':
    case 'U':

    // eCommands::COMMAND_MOVE_DOWN
    case 'd':
    case 'D':

    // eCommands::COMMAND_MOVE_LEFT
    case 'l':
    case 'L':

    // eCommands::COMMAND_MOVE_RIGHT
    case 'r':
    case 'R':

    // eCommands::COMMAND_WAIT
    case 'w':
    case 'W':

    // eCommands:: COMMAND_REPEAT
    case 'b':
    case 'B':

    // eCommands::COMMAND_MOVETO_X
    case 'x':
    case 'X':

    // eCommands::COMMAND_MOVETO_Y
    case 'y':
    case 'Y':

    // eCommands::COMMAND_FACE
    case 'f':
    case 'F':

    // eCommands::COMMAND_KILL
    case 'k':
    case 'K':

    // eCommands::COMMAND_FINISH_COMMANDS
    case '\0':

        return true;
    }
}

KMovement::eStatus KMovement::minimize_path(std::string& out_buffer, const std::string& source)
{
    const size_t sourceLength = source.length();
    out_buffer.clear();

    if (sourceLength == 0)
    {
        return eStatus::PATH_FOUND;
    }

    size_t source_index = 0;
    while (source_index < sourceLength)
    {
        // Get the first character, such as the first 'U' in "UUUUU"
        char value = source[source_index];
        source_index++;

        // Prevent invalid commands from making it into the output.
        if (!isValidCommand(value))
        {
            continue;
        }

        uint32_t repetition = 1;
        // While the next characters are the same as 'value' above, increment repetition.
        // "UUUUU" will have a value 'U' with repetition of 5.
        while (source_index < sourceLength && source[source_index] == value)
        {
            source_index++;
            repetition++;
        }

        // Output format: the command followed by repetitions: "U5" or "D1"
        std::stringstream ss;
        ss << value << repetition;
        out_buffer.append(ss.str());
    }

    return eStatus::PATH_FOUND;
}

KMovement::eStatus KMovement::search_paths(int32_t* map, const uint32_t entity_id, const uint32_t recursion_count, const uint32_t source_x, const uint32_t source_y, const uint32_t target_x, const uint32_t target_y)
{
    if (map == nullptr)
    {
        return eStatus::UNKNOWN_ERROR;
    }

    eStatus result = eStatus::NO_PATH_FOUND;
    const size_t index = source_y * g_map.xsize + source_x;
    const int32_t value = map[index];

    if ((value != kImpassibleBlock) &&
        (value == kNotYetProcessed || value > (int32_t)recursion_count) &&
        (recursion_count == 1 || !kEntity.entityat(source_x, source_y, entity_id)))
    {
        map[index] = recursion_count;

        if (source_x == target_x && source_y == target_y)
        {
            return eStatus::PATH_FOUND;
        }

        if (source_x > 0)
        {
            eStatus recursiveResult = search_paths(map, entity_id, recursion_count + 1, source_x - 1, source_y, target_x, target_y);
            if (recursiveResult == eStatus::PATH_FOUND)
            {
                result = eStatus::PATH_FOUND;
            }
        }

        if (source_x < g_map.xsize - 1)
        {
            eStatus recursiveResult = search_paths(map, entity_id, recursion_count + 1, source_x + 1, source_y, target_x, target_y);
            if (recursiveResult == eStatus::PATH_FOUND)
            {
                result = eStatus::PATH_FOUND;
            }
        }

        if (source_y > 0)
        {
            eStatus recursiveResult = search_paths(map, entity_id, recursion_count + 1, source_x, source_y - 1, target_x, target_y);
            if (recursiveResult == eStatus::PATH_FOUND)
            {
                result = eStatus::PATH_FOUND;
            }
        }

        if (source_y < g_map.ysize - 1)
        {
            eStatus recursiveResult = search_paths(map, entity_id, recursion_count + 1, source_x, source_y + 1, target_x, target_y);
            if (recursiveResult == eStatus::PATH_FOUND)
            {
                result = eStatus::PATH_FOUND;
            }
        }
    }

    return (result);
}
