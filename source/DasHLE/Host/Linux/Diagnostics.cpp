#include "DasHLE/Support/Diagnostics.h"

#include <cstdio>

using namespace dashle;

// Diagnostics

void _impl::logLine(const std::string& s) { std::printf("%s\n", s.c_str()); }