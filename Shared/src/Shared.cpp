#include "Shared.h"

#include <cstdarg>
#include <cstdio>

void WriteError(const char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    std::vfprintf(stderr, msg, args);
    va_end(args);
}