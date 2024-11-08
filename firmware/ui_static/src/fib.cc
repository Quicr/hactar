#include "fib.hh"

uint32_t FibInt32(const uint32_t n)
{
    if (n > 2)
    {
        return FibInt32(n-1) + FibInt32(n-2);
    }

    return (uint32_t)1;
}

int64_t FibInt64(const uint32_t n)
{
    if (n > 2)
    {
        return FibInt64(n-1) + FibInt64(n-2);
    }

    return (int64_t)1;
}

float FibFloat32(const uint32_t n)
{
    if (n > 2)
    {
        return FibFloat32(n-1) + FibFloat32(n-2);
    }

    return 1.0f;
}

double FibFloat64(const uint32_t n)
{
    if (n > 2)
    {
        return FibFloat64(n-1) + FibFloat64(n-2);
    }

    return 1.0;
}