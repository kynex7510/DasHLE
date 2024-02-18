#ifndef _DASHLE_HOST_FS_H
#define _DASHLE_HOST_FS_H

#include "DasHLE/Support/Types.h"

#include <vector>
#include <fstream>
#include <filesystem>

namespace dashle::host::fs {

using namespace std::filesystem;

inline Expected<void> readFile(const path& path, std::vector<u8>& buffer) {
    std::ifstream fileHandle(path, std::ios::ate | std::ios::binary);
    if (!fileHandle.is_open())
        return Unexpected(Error::OpenFailed);

    buffer.resize(fileHandle.tellg());
    fileHandle.seekg(0, fileHandle.beg);
    fileHandle.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
    return EXPECTED_VOID;
}

} // dashle::host::fs

#endif /* _DASHLE_HOST_FS_H */