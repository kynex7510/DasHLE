#include "DasHLE/Base.h"

#include <cstdio>

using namespace dashle;

// Base

void _internal::breakExec() { std::abort(); }

void _internal::logLine(const std::string& s) { std::printf("%s\n", s.c_str()); }