/*! \file
 * \brief Specifics for Windows
 *
 * \author PH
 * \date 20030723
 *
 * This includes any bits which are specific for Windows platforms
 */

#include "kq.h"
#include "platform.h"
#include <allegro.h>

#include <cstdarg>
#include <string>

// This clashes with a Windows typedef
#undef PSIZE
#include <winalleg.h>

typedef HRESULT(WINAPI* SHGETFOLDERPATH)(HWND, int, HANDLE, DWORD, LPWSTR);

#define CSIDL_FLAG_CREATE 0x8000
#define CSIDL_APPDATA 0x1A
#define SHGFP_TYPE_CURRENT 0

namespace KqFork
{
bool bAreUserAndGameDirsInitialized = false;
std::string userDir;
std::string gameDir;

/**
 * Combine an arbitrary number of strings into a "/" delimited path.
 *
 * This will not escape spaces, so cases such as
 *   "/home/user name/subdir"
 * will have to be handled by the caller.
 *
 * @param numStringsToCombine How many strings to expect to follow.
 * @param[...] Variable number of std::string entries.
 * @return A single string with "/" delimiter between entries.
 */
std::string CombinePaths(size_t numStringsToCombine, ...)
{
    std::string constructedPath = "";

    if (numStringsToCombine == 0)
    {
        return constructedPath;
    }

    va_list args;
    va_start(args, numStringsToCombine);
    constructedPath += va_arg(args, std::string);
    for (size_t i = 1; i < numStringsToCombine; i++)
    {
        constructedPath += "/";
        constructedPath += va_arg(args, std::string);
    }

    return constructedPath;
}

} // namespace KqFork

/*! \brief Returns the full path for this file
 *
 * This function first checks if the file can be found in the user's
 * directory. If it can not, it checks the relevant game directory
 * (data, music, lib, etc)
 *
 * \param str1 The first part of the string, assuming the file can't be found in user_dir (eg. "C:\Program Files\KQ\")
 * \param str2 The second part of the string (eg. "scripts")
 * \param file The filename
 * \returns the combined path
 */
std::string get_resource_file_path(const std::string str1, const std::string str2, const std::string file)
{
    FILE* fp = nullptr;

    const std::string userDirFound = KqFork::CombinePaths(3, KqFork::userDir, str2, file);
    fp = fopen(userDirFound.c_str(), "r");
    if (fp)
    {
        fclose(fp);
        return userDirFound;
    }

    const std::string customDirFound = KqFork::CombinePaths(3, str1, str2, file);
    fp = fopen(customDirFound.c_str(), "r");
    if (fp)
    {
        fclose(fp);
        return customDirFound;
    }
    return "";
}

/*! \brief Returns the full path for this lua script.
 *
 * This function first checks if the lua file can be found in the user's directory. If it
 * can't, it checks the relevant game directory (data, music, lib, etc). For each
 * directory, it first checks for a lob file, and then it checks for a lua file. This
 * function is similar to get_resource_file_path(), but takes special considerations for
 * lua files. Whereas get_resource_file_path() takes the full filename (eg. "main.map"),
 * this function takes the filename without extension (eg "main").
 *
 * \param file The filename
 * \returns the combined path
 */
std::string get_lua_file_path(const std::string file)
{
    FILE* fp = nullptr;

    const std::string userDirLob = KqFork::CombinePaths(3, KqFork::userDir, std::string("scripts"), file + ".lob");
    fp = fopen(userDirLob.c_str(), "r");
    if (fp)
    {
        fclose(fp);
        return userDirLob;
    }

    const std::string userDirLua = KqFork::CombinePaths(3, KqFork::userDir, std::string("scripts"), file + ".lua");
    fp = fopen(userDirLua.c_str(), "r");
    if (fp)
    {
        fclose(fp);
        return userDirLua;
    }

    const std::string gameDirLob = KqFork::CombinePaths(3, KqFork::gameDir, std::string("scripts"), file + ".lob");
    fp = fopen(gameDirLob.c_str(), "r");
    if (fp)
    {
        fclose(fp);
        return gameDirLob;
    }

    const std::string gameDirLua = KqFork::CombinePaths(3, KqFork::gameDir, std::string("scripts"), file + ".lua");
    fp = fopen(gameDirLua.c_str(), "r");
    if (fp)
    {
        fclose(fp);
        return gameDirLua;
    }

    return "";
}

/*! \brief Return the name of 'significant' directories.
 *
 * \param   dir Enumerated constant for directory type  \sa DATA_DIR et al.
 * \param   file File name below that directory.
 * \returns the combined path
 */
std::string kqres(eDirectories dir, const std::string& file)
{
    HINSTANCE SHFolder;
    SHGETFOLDERPATH SHGetFolderPath;

    if (!KqFork::bAreUserAndGameDirsInitialized)
    {
        WCHAR tmp[MAX_PATH];
        std::string home;
        /* Get home directory; this bit originally written by SH */
        SHFolder = LoadLibrary("shfolder.dll");
        if (SHFolder != nullptr)
        {
            SHGetFolderPath = (SHGETFOLDERPATH)GetProcAddress(SHFolder, "SHGetFolderPathW");
            if (SHGetFolderPath != nullptr)
            {
                /* Get the "Application Data" folder for the current user */
                if (SHGetFolderPath(nullptr, CSIDL_APPDATA | CSIDL_FLAG_CREATE, nullptr, SHGFP_TYPE_CURRENT, tmp) == S_OK)
                {
                    home.append(uconvert((const char*)tmp, U_UNICODE, nullptr, U_UTF8, 0));
                }
            }
            FreeLibrary(SHFolder);
        }

        /* Do not get fooled by a corrupted $HOME */
        if (home.length() > 0 && home.length() < MAX_PATH)
        {
            KqFork::userDir = KqFork::CombinePaths(2, home, std::string("KQ"));
            /* Always try to make the directory, just to be sure. */
            _mkdir(KqFork::userDir.c_str());
        }
        else
        {
            KqFork::userDir = ".";
        }
        /* Now the data directory */
        KqFork::gameDir = ".";
        KqFork::bAreUserAndGameDirsInitialized = true;
    }

    switch (dir)
    {
    case DATA_DIR:
        return get_resource_file_path(KqFork::gameDir, std::string("data"), file);
        break;
    case MAP_DIR:
        return get_resource_file_path(KqFork::gameDir, std::string("maps"), file);
        break;
    case SAVE_DIR:
    case SETTINGS_DIR:
        return KqFork::CombinePaths(2, KqFork::userDir, file);
        break;
    case MUSIC_DIR:
        return get_resource_file_path(KqFork::gameDir, std::string("music"), file);
        break;
    case SCRIPT_DIR:
        return get_lua_file_path(file);
        break;
    default:
        return nullptr;
    }
}
