#pragma once

class Display
{
    virtual void Initialize() = 0;
    virtual void Render() = 0;
    virtual void Update() = 0;
    virtual void Reset() = 0;
};