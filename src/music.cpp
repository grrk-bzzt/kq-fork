/*! \file
 * \brief In-game music routines
 *
 * Handles playing and pausing music in the game.
 * Interfaces to DUMB
 */

#include <stdio.h>
#include <string.h>
#include <string>

#include "kq.h"
#include "music.h"
#include "platform.h"

/* DUMB version of music */
#include <aldumb.h>

/* private variables */
#define MAX_MUSIC_PLAYERS 3
static DUH* mod_song[MAX_MUSIC_PLAYERS];
static AL_DUH_PLAYER* mod_player[MAX_MUSIC_PLAYERS];
static int current_music_player;

/*! \brief Initiate music player (DUMB)
 *
 * Initializes the music players. Must be called before any other
 * music function. Needs to be shutdown when finished.
 */
void KMusic::init_music()
{
    atexit(&dumb_exit);
    dumb_register_stdfiles();
    dumb_resampling_quality = 2;

    /* initialize all music players */
    current_music_player = MAX_MUSIC_PLAYERS;
    while (current_music_player--)
    {
        mod_song[current_music_player] = nullptr;
        mod_player[current_music_player] = nullptr;
    }
    current_music_player = 0;
}

/*! \brief Clean up and shut down music (DUMB)
 *
 * Performs any cleanup needed. Must be called before the program exits.
 */
void KMusic::shutdown_music()
{
    if (is_sound != 0)
    {
        do
        {
            stop_music();
        }
        while (current_music_player--);
    }
}

/*! \brief Set the music volume (DUMB)
 *
 * Sets the volume of the currently playing music.
 *
 * \param   volume 0 (silent) to 100 (loudest)
 */
void KMusic::set_music_volume(float volume)
{
    if (is_sound != 0 && mod_player[current_music_player])
    {
        al_duh_set_volume(mod_player[current_music_player], volume);
    }
}

/*! \brief Poll the music (DUMB)
 *
 * Does whatever is needed to ensure the music keeps playing.
 * It's safe to call this too much, but shouldn't be called inside a timer.
 */
void KMusic::poll_music()
{
    if (is_sound != 0)
    {
        al_poll_duh(mod_player[current_music_player]);
    }
}

/*! \brief Play a specific song (DUMB)
 *
 * This will stop any currently played song, and then play
 * the requested song.  Based on the extension given, the appropriate player
 * is called.
 *
 * \param   music_name The relative filename of the song to be played
 * \param   position The position of the file to begin at
 */
void KMusic::play_music(const std::string& music_name, long position)
{
    if (is_sound == 0)
    {
        return;
    }

    std::string fstr = kqres(MUSIC_DIR, music_name);

    stop_music();
    if (exists(fstr.c_str()))
    {
        if (strstr(fstr.c_str(), ".mod"))
        {
            mod_song[current_music_player] = dumb_load_mod(fstr.c_str());
        }

        else if (strstr(fstr.c_str(), ".xm"))
        {
            mod_song[current_music_player] = dumb_load_xm(fstr.c_str());
        }

        else if (strstr(fstr.c_str(), ".s3m"))
        {
            mod_song[current_music_player] = dumb_load_s3m(fstr.c_str());
        }

        else
        {
            mod_song[current_music_player] = nullptr;
        }
        if (mod_song[current_music_player])
        {
            /* ML: we should (?) adjust the buffer size after everything is running
                * smooth */
            float vol = float(gmvol) / 250.0f;
            mod_player[current_music_player] = al_start_duh(mod_song[current_music_player], 2, position, vol, 4096 * 4, 44100);
        }
        else
        {
            TRACE(_("Could not load %s!\n"), fstr.c_str());
        }
    }
    else
    {
        mod_song[current_music_player] = nullptr;
    }
}

/*! \brief Stop the music (DUMB)
 *
 * Stops any music being played. To start playing more music, you
 * must call play_music(), as the current music player will no longer
 * be available and the song unloaded from memory.
 */
void KMusic::stop_music()
{
    if (is_sound != 0 && mod_player[current_music_player])
    {
        al_stop_duh(mod_player[current_music_player]);
        unload_duh(mod_song[current_music_player]);
        mod_player[current_music_player] = nullptr;
        mod_song[current_music_player] = nullptr;
    }
}

/*! \brief Pauses the current music file (DUMB)
 *
 * Pauses the currently playing music file. It may be resumed
 * by calling resume_music(). Pausing the music file may be used
 * to nest music (such as during a battle).
 */
void KMusic::pause_music()
{
    if (is_sound != 0)
    {
        if (current_music_player < MAX_MUSIC_PLAYERS - 1)
        {
            al_pause_duh(mod_player[current_music_player]);
            current_music_player++;
        }
        else
        {
            TRACE(_("reached maximum levels of music pauses!\n"));
        }
    }
}

/*! \brief Resume paused music (DUMB)
 *
 * Resumes the most recently paused music file. If a call to
 * play_music() was made in between, that file will be stopped.
 */
void KMusic::resume_music()
{
    if (is_sound != 0 && current_music_player > 0)
    {
        stop_music();
        current_music_player--;
        al_resume_duh(mod_player[current_music_player]);
    }
}

KMusic Music;
