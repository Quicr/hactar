#ifndef STATE_H
#define STATE_H

enum State
{
    Error,
    Waiting,
    Reset,
    Running,
    UI_Upload_Reset,
    UI_Upload,
    Net_Upload_Reset,
    Net_Upload,
    Debug_Reset,
    UI_Debug_Reset,
    Net_Debug_Reset,
    Debug_Running
};

#endif