#include "Base.h"

#include <cstdio>

// Base

std::string dashle::errorAsString(Error error) {
#define ERROR_CASE(e) \
    case Error::e:    \
        return #e;

    switch (error) {
        ERROR_CASE(OpenFailed);
        ERROR_CASE(InvalidArch);
        ERROR_CASE(InvalidObject);
        ERROR_CASE(NoSegments);
        ERROR_CASE(InvalidSegments);
        ERROR_CASE(NoMemory);
        ERROR_CASE(RelocationFailed);
        ERROR_CASE(InvalidSize);
        ERROR_CASE(InvalidAddress);
        ERROR_CASE(NoVirtualMemory);
        ERROR_CASE(NoHostMemory);
        ERROR_CASE(NotFound);
    }

#undef ERROR_CASE

    DASHLE_UNREACHABLE("Invalid enum value: {}", static_cast<u32>(error));
}