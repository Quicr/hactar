#include "app_main.hh"
#include <cstdio>
#include <cstdlib>

extern "C" void Error(const char* who, const char* why)
{
    std::fprintf(stderr, "UI error in %s: %s\n", who, why);
    std::abort();
}
