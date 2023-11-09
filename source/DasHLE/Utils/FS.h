#ifndef _DASHLE_UTILS_FS_H
#define _DASHLE_UTILS_FS_H

#include "DasHLE/Base.h"

#include <vector>
#include <fstream>
#include <filesystem>

namespace dashle::utils::fs {

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

} // dashle::utils::fs

#endif /* _DASHLE_UTILS_FS_H */