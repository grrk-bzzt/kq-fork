#pragma once

#include <string>

/* ML: include aldumb.h here, because other modules really don't know that
   we are using dumb, and it doesn't make sense to have to include aldumb
   individually */

class KMusic
{
public:
    void init_music();
    void shutdown_music();
    void poll_music();

    void set_music_volume(float volume);

    void play_music(const std::string& music_name, long position);
    void pause_music();
    void resume_music();
    void stop_music();
};

extern KMusic Music;
