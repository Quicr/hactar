#ifndef STATE_H
#define STATE_H

enum State
{
    Error,
    Running,
    UI_Upload,
    Net_Upload,
    Normal,
    Debug,
    UI_Debug,
    Net_Debug,
    Loopback,
    Configator
};

#endif