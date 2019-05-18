#pragma once

#include <stdint.h>
#include <stdlib.h>

class KMovement
{
public:
    enum class eStatus
    {
        PATH_FOUND,
        NO_PATH_FOUND,
        OUT_OF_MEMORY,
        UNKNOWN_ERROR
    };

    KMovement();
    ~KMovement() = default;

    /**
     * Path search implementation for KQ
     *
     * Calculate the shortest path between a given NPC and a target point.
     *
     * @param[out] out_buffer A buffer where the result will be stored.
     * @param[in] entity_id The ID of the entity moving around.
     * @param[in] source_x  The x coordinate of the source point.
     * @param[in] source_y  The y coordinate of the source point.
     * @param[in] target_x  The x coordinate of the target point.
     * @param[in] target_y  The y coordinate of the target point.
     *
     * @return eStatus status of whether it could find the path.
     */
    eStatus find_path(std::string& out_buffer, size_t entity_id, uint32_t source_x, uint32_t source_y, uint32_t target_x, uint32_t target_y);

private:
    /*! \brief Set impassible tile values to -1.
     *
     * All tiles containing an obstacle or an entity are set to -1.
     *
     * Tiles that have not been evaluated default to 0.
     * Otherwise, the weight (distance between source and destination) is calculated,
     * where the smallest value is considered most optimal.
     *
     * @param[out] map The map where the result will be copied
     */
    void mark_obstacles_in_map(int32_t* map);

    /*! \brief Internal path search routine.
     *
     * This function uses recursion to find the shortest path to the target
     * point. Once it returns 0, a path was successfully found.
     *
     * @param[in,out] map   The map in where to write the paths.
     * @param[in] entity_id The ID of the entity moving around.
     * @param[in] recursion_count How many times this method has been called recursively.
     * @param[in] source_x  The x coordinate of the source point.
     * @param[in] source_y  The y coordinate of the source point.
     * @param[in] target_x  The x coordinate of the target point.
     * @param[in] target_y  The y coordinate of the target point.
     *
     * \returns 0 if a path was successfully found, otherwise non-zero to indicate
     * failure.
     */
    eStatus search_paths(int32_t* map, const uint32_t entity_id, const uint32_t recursion_count, const uint32_t source_x, const uint32_t source_y, const uint32_t target_x, const uint32_t target_y);

    /*! \brief Generates the solution path.
     *
     * The only way of doing this is walking the path backwards, minimizing and
     * turning it the way around.
     *
     * @param[out] out_buffer Buffer to store the solution.
     * @param[in] map      The map with the paths.
     * @param[in] target_x Target x coordinate.
     * @param[in] target_y Target y coordinate.
     *
     * \returns If the solution was copied.
     *          0 The solution was copied.
     *          1 The buffer was too small for the solution to be copied.
     *          2 There was no solution, or internal error.
     */
    eStatus compose_path(std::string& out_buffer, const int32_t* map, uint32_t target_x, uint32_t target_y);

    /**
     * Return whether a character is a valid command, such as 'U' or 'K'.
     *
     * Currently, only letters are valid commands, with digits to indicate repetitions of
     * those commands: "U4" is "UUUU" when condensed.
     */
    bool isValidCommand(const char input);

    /*! \brief Minimizes a path.
     *
     * Given a path like "RRRRDRRDLU", this functions generates "R4D1R2D1L1U1".
     *
     * @param[out] out_buffer The buffer where the result will be stored.
     * @param[in] source   The original string.
     *
     * \returns 0 if the solution was copied,
     *          1 if the buffer was too small for the solution to be copied.
     */
    eStatus minimize_path(std::string& out_buffer, const std::string& source);
};

extern KMovement kMovement;
