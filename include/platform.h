#pragma once

/*! \file
 * \brief Platform-specific interfaces, prototypes, and #defines
 * \author OC
 * \date 20100221
 */

#include <string>

enum eDirectories
{
    DATA_DIR = 0,
    MAP_DIR = 1,
    SAVE_DIR = 2,
    MUSIC_DIR = 3,
    SCRIPT_DIR = 4,
    SETTINGS_DIR = 5,
};

/* Get the directory for application data (music, gfx, etc.)
   or user data (screenshots, prefs, saved games)
 */
std::string kqres(enum eDirectories, const std::string& file);

std::string get_lua_file_path(const std::string, const std::string);
std::string get_resource_file_path(const std::string, const std::string, const std::string);

#define maybe_poll_joystick poll_joystick
