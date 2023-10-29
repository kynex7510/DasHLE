#ifndef _DASHLE_BINARY_H
#define _DASHLE_BINARY_H

#include "Memory.h"

#include <span>

namespace dashle::binary {

enum class Error {
    Success,
    OpenFailed,
    UnknownArch,
    InvalidObject,
    NoSegments,
    InvalidSegments,
    NoMemory,
    RelocationFailed,
};

enum class Type {
    Unknown,
    ARM_LE,
    Aarch64_LE,
};

class Binary {
    memory::Allocator &m_Allocator;
    Type m_Type = Type::Unknown;
    std::span<u8> m_CodeBuffer;

    template <typename CFG>
    Error loaderImpl(const std::span<const u8> buffer);

protected:
    virtual bool fixDataRelocation() = 0;
    virtual bool fixCodeRelocation() = 0;

    Binary(memory::Allocator &allocator) : m_Allocator(allocator) {}

public:
    Error load(const stdfs::path &path);

    memory::Allocator &allocator() {
        return m_Allocator;
    }

    Type type() const {
        return m_Type;
    }

    std::span<u8> codeBuffer() {
        return m_CodeBuffer;
    }

    std::uintptr_t baseAddress() const {
        return reinterpret_cast<std::uintptr_t>(m_CodeBuffer.data());
    }

    std::size_t size() const {
        return m_CodeBuffer.size_bytes();
    }

    // TODO
    std::uintptr_t symbolAddress(const std::string &sym) const {
        return 0;
    }
};

std::string errorAsString(Error error);
std::string typeAsString(Type type);

} // namespace dashle::binary

#endif /* _GDACL_BINARY_H */