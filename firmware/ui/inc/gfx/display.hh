#pragma once

#include "shapes/shape.hh"

class Display
{
public:
    virtual void Initialize() = 0;
    virtual void Render() = 0;
    virtual void Update() = 0;
    virtual void Reset() = 0;
    virtual void PushShape(Shape shape) = 0;
};