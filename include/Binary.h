#ifndef _GDACL_BINARY_H
#define _GDACL_BINARY_H

#include <elf.h>

#include <filesystem>
#include <memory>
#include <span>

namespace gdacl::binary {

namespace stdfs = std::filesystem;

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

class Base {
    Type m_Type = Type::Unknown;
    std::span<std::uint8_t> m_CodeBuffer;

protected:
    virtual std::uintptr_t resolveDependency(const std::string &sym) = 0;

    Base() {}

public:
    Error load(const stdfs::path &path);

    Type getType() const {
        return m_Type;
    }

    std::uintptr_t getBaseAddress() const {
        return reinterpret_cast<std::uintptr_t>(m_CodeBuffer.data());
    }

    std::size_t getSize() const {
        return m_CodeBuffer.size_bytes();
    }

    std::uintptr_t getSymbolAddress(const std::string &sym) const;
};

std::string getErrorAsString(Error error);
std::string getTypeAsString(Type type);

} // namespace gdacl::binary

#endif /* _GDACL_BINARY_H */