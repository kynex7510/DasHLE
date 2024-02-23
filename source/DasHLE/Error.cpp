#include "DasHLE/Support/Types.h"

std::string dashle::errorAsString(Error error) {
#define ERROR_CASE(e) \
    case Error::e:    \
        return #e;

    switch (error) {
        ERROR_CASE(OpenFailed);
        ERROR_CASE(InvalidSize);
        ERROR_CASE(InvalidAlignment);
        ERROR_CASE(InvalidMagic);
        ERROR_CASE(InvalidClass);
        ERROR_CASE(InvalidDataEncoding);
        ERROR_CASE(NoPIE);
        ERROR_CASE(InvalidArch);
        ERROR_CASE(InvalidRelocation);
        ERROR_CASE(NoSegments);
        ERROR_CASE(NoSections);
        ERROR_CASE(InvalidSegment);
        ERROR_CASE(RelocationFailed);
        ERROR_CASE(InvalidAddress);
        ERROR_CASE(InvalidIndex);
        ERROR_CASE(NoVirtualMemory);
        ERROR_CASE(NoHostMemory);
        ERROR_CASE(NotFound);
        ERROR_CASE(InvalidArgument);
        ERROR_CASE(Duplicate);
        ERROR_CASE(InvalidOperation);
    }

#undef ERROR_CASE

    DASHLE_UNREACHABLE("Invalid enum value: {}", static_cast<u32>(error));
}