#pragma once

class KPlayerInput
{
public:
    KPlayerInput();
    ~KPlayerInput();

    void readcontrols();

public:
    // Flags for determining keypresses and player movement.
    int right, left, up, down;
    int besc, balt, bctrl, benter;
    int bhelp;
    int bcheat;

    // Scan codes for the keys (help is always F1)
    int kright, kleft, kup, kdown;
    int kesc, kenter, kalt, kctrl;

    // Joystick buttons
    int jbalt, jbctrl, jbenter, jbesc;
};

extern KPlayerInput PlayerInput;
