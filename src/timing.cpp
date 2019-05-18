/*! \file
 * \brief Timing handler functions
 * \author ML
 * \date 20102002
 *
 * Looks after keeping the music playing whilst the game is 'paused'
 */
#ifdef _WIN32
#include <allegro.h>
#include <stdarg.h>
#include <winalleg.h>
#endif

#include "kq.h"
#include "music.h"
#include "timing.h"

static int mfrate;
static int frate;

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#include <sys/time.h>

/*! \brief Pause the game for a period of time
 *
 * Calls poll_music() continuously to ensure music keeps playing.
 * PH renamed from wait to kq_wait as the former was causing conflicts on OSX
 *
 * \param   ms Time to pause in milliseconds
 */
void kq_wait(long ms)
{
    /* dumb's doc says to call poll_music each bufsize / freq seconds */
    static const int delay = 1000 * 4096 * 2 / 44100;
    struct timeval timeout = {0, 0};
    while (ms > 0)
    {
        if (ms > delay)
        {
            timeout.tv_usec = delay * 1000;
            ms -= delay;
        }
        else
        {
            timeout.tv_usec = ms * 1000;
            ms = 0;
        }
        select(0, nullptr, nullptr, nullptr, &timeout);

        poll_music();
    }
}

/*! \brief Sleep to limit the frame rate
 *
 * Calculates the time since the last call and
 * waits the remaining time for the next scheduled
 * screen update.
 *
 * \param   fps The targeted frames per second
 * \returns The actual frames per second
 */
int limit_frame_rate(int fps)
{
    static struct timeval last_exec = {0, 0};
    struct timeval tv = {0, 0};
    struct timeval timeout = {0, 0};
    time_t seconds;

    gettimeofday(&tv, 0);
    /* The time between now and (last exec + delay) */
    timeout.tv_usec = last_exec.tv_usec - tv.tv_usec + (1000000 / fps) + 1000000 * (last_exec.tv_sec - tv.tv_sec);
    seconds = last_exec.tv_sec;
    /* Negative waits are not yet possible */
    if (timeout.tv_usec < 0 || !last_exec.tv_sec)
    {
        last_exec.tv_usec = tv.tv_usec;
        last_exec.tv_sec = tv.tv_sec;
    }
    else
    {
        select(0, nullptr, nullptr, nullptr, &timeout);
        last_exec.tv_usec += (1000000 / fps);
        if (last_exec.tv_usec > 1000000)
        {
            ++last_exec.tv_sec;
            last_exec.tv_usec -= 1000000;
        }
    }
    if (seconds != last_exec.tv_sec)
    {
        mfrate = frate;
        frate = 0;
    }
    ++frate;
    return mfrate;
}

#elif defined(_WIN32)

void kq_wait(long ms)
{
    /* dumb's doc says to call poll_music each bufsize / freq seconds */
    static const int delay = 1000 * 4096 * 4 / 44100;

    while (ms > 0)
    {
        if (ms > delay)
        {
            Sleep(delay);
            ms -= delay;
        }
        else
        {
            Sleep(ms);
            ms = 0;
        }
        Music.poll_music();
    }
}

int limit_frame_rate(int fps)
{
    static DWORD last_exec;
    static bool initialized = false;
    DWORD now = GetTickCount();
    if (!initialized)
    {
        initialized = true;
        last_exec = now;
    }
    DWORD next_exec = last_exec + 1000 / fps;

    // Sleep if current time is before next due time.
    if (now < next_exec)
    {
        Sleep(next_exec - now);
    }
    if (now / 1000 != last_exec / 1000)
    {
        mfrate = frate;
        frate = 0;
    }
    last_exec = now;
    ++frate;
    return mfrate;
}
#else

#include <allegro.h>

/*! \brief Poll the music system
 *
 * call poll_music() to ensure that music plays
 * \remark PH does this need locking with LOCK_FUNCTION
 *            like a timer function does?
 */
static void _kq_rest_callback() { Music.poll_music(); }

void kq_wait(long ms) { rest_callback(ms, _kq_rest_callback); }

int limit_frame_rate(int fps)
{
    fps = fps; // prevent "unused param" warnings
    static int last_ksec = 0;

    vsync();
    ++frate;
    if (last_ksec != ksec)
    {
        last_ksec = ksec;
        mfrate = frate;
        frate = 0;
    }
    return mfrate;
}

#endif // HAVE_SYS_SELECT_H
