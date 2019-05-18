#include "allegro.h"
#include "input.h"
#include "kq.h"
#include "music.h"
#include "platform.h"

KPlayerInput::KPlayerInput()
{

}

KPlayerInput::~KPlayerInput()
{

}

/*! \brief Handle user input.
*
* Updates all of the game controls according to user input.
* 2003-05-27 PH: updated to re-enable the joystick
* 2003-09-07 Edge <hardedged@excite.com>: removed duplicate input, joystick code
* 2003-09-07 Caz Jones: last time code workaround pci-gameport bug
*            (should not affect non-buggy drivers - please report to edge)
*/
void KPlayerInput::readcontrols()
{
    JOYSTICK_INFO* stk = nullptr;

    Music.poll_music();

    /* PH 2002.09.21 in case this is needed (not sure on which platforms it is) */
    if (keyboard_needs_poll())
    {
        poll_keyboard();
    }

    PlayerInput.balt = key[PlayerInput.kalt];
    PlayerInput.besc = key[PlayerInput.kesc];
    PlayerInput.bctrl = key[PlayerInput.kctrl];
    PlayerInput.benter = key[PlayerInput.kenter];
    PlayerInput.bhelp = key[KEY_F1];
    PlayerInput.bcheat = key[KEY_F10];

    PlayerInput.up = key[PlayerInput.kup];
    PlayerInput.down = key[PlayerInput.kdown];
    PlayerInput.left = key[PlayerInput.kleft];
    PlayerInput.right = key[PlayerInput.kright];

    /* Emergency kill-game set. */
    /* PH modified - need to hold down for 0.50 sec */
    if (key[KEY_ALT] && key[KEY_X])
    {
        int kill_time = timer_count + Game.KQ_TICKS / 2;

        while (key[KEY_ALT] && key[KEY_X])
        {
            if (timer_count >= kill_time)
            {
                /* Pressed, now wait for release */
                clear_bitmap(screen);
                while (key[KEY_ALT] && key[KEY_X])
                {
                }
                Game.program_death(_("X-ALT pressed... exiting."));
            }
        }
    }
#ifdef DEBUGMODE
    extern char debugging;
    if (debugging > 0)
    {
        if (key[KEY_F11])
        {
            Game.data_dump();
        }

        /* Back to menu - by pretending all the heroes died.. hehe */
        if (key[KEY_ALT] && key[KEY_M])
        {
            alldead = 1;
        }
    }
#endif

    if (use_joy > 0 && maybe_poll_joystick() == 0)
    {
        stk = &joy[use_joy - 1];
        PlayerInput.left |= stk->stick[0].axis[0].d1;
        PlayerInput.right |= stk->stick[0].axis[0].d2;
        PlayerInput.up |= stk->stick[0].axis[1].d1;
        PlayerInput.down |= stk->stick[0].axis[1].d2;

        PlayerInput.balt |= stk->button[0].b;
        PlayerInput.bctrl |= stk->button[1].b;
        PlayerInput.benter |= stk->button[2].b;
        PlayerInput.besc |= stk->button[3].b;
    }
}

KPlayerInput PlayerInput;
