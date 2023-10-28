#include "GDACL.h"
#include "ELF.h"

#include <fstream>
#include <vector>

using namespace gdacl;
using namespace binary;

// Config

struct ConfigARM : public elf::Config32, public elf::ConfigLE {
    static constexpr auto ARCH = EM_ARM;
    static constexpr auto BINARY_TYPE = Type::ARM_LE;
};

struct ConfigAarch64 : public elf::Config64, public elf::ConfigLE {
    static constexpr auto ARCH = EM_AARCH64;
    static constexpr auto BINARY_TYPE = Type::Aarch64_LE;
};

ELF_ASSERT_CONFIG(ConfigARM);
ELF_ASSERT_CONFIG(ConfigAarch64);

// Binary

struct BinaryData {
    Type type;
    std::span<std::uint8_t> codeBuffer;
};

constexpr std::size_t alignSize(std::size_t size, std::size_t align) {
    return (size + (align - 1)) & ~(align - 1);
}

template <elf::ConfigType CFG>
static std::size_t getSegmentsAllocSize(const elf::Segments<CFG> &segments) {
    std::size_t allocSize = 0;

    for (const auto segment : segments) {
        if (segment->p_memsz < segment->p_filesz) {
            allocSize = 0;
            break;
        }

        if (segment->p_align > 1)
            allocSize += alignSize(segment->p_memsz, segment->p_align);
        else
            allocSize += segment->p_memsz;
    }

    return allocSize;
}

template <elf::ConfigType CFG>
static Error loaderImpl(BinaryData &binaryData, const std::span<const std::uint8_t> buffer) {
    auto header = elf::getHeader<CFG>(buffer);
    if (!header)
        return Error::InvalidObject;

    // Calculate allocation space for load segments.
    auto loadSegments = elf::getSegments<CFG>(header, PT_LOAD);
    if (loadSegments.empty())
        return Error::NoSegments;

    auto const regionSize = getSegmentsAllocSize<CFG>(loadSegments);
    if (!regionSize)
        return Error::InvalidSegments;

    // Allocate memory and map segments.
    auto p = new (std::nothrow) std::uint8_t[regionSize];
    if (!p)
        return Error::NoMemory;

    for (auto const segment : loadSegments) {
        std::copy(
            buffer.data() + segment->p_offset,
            buffer.data() + segment->p_offset + segment->p_filesz,
            p + segment->p_vaddr);
    }

    // Apply relocations.
    // TODO

    binaryData.type = CFG::BINARY_TYPE;
    binaryData.codeBuffer = std::span{p, regionSize};
    return Error::Success;
}

Error Base::load(const stdfs::path &path) {
    // Read binary.
    std::vector<std::uint8_t> buffer;
    std::ifstream fileHandle(path, std::ios::ate | std::ios::binary);
    if (!fileHandle.is_open())
        return Error::OpenFailed;

    buffer.resize(fileHandle.tellg());
    fileHandle.seekg(0, fileHandle.beg);
    fileHandle.read(reinterpret_cast<char *>(buffer.data()), buffer.size());

    // Load binary in memory.
    BinaryData binaryData = {};
    Error error = Error::UnknownArch;

#define CFG_ARCH_CASE(cfg)                           \
    case cfg::ARCH:                                  \
        error = loaderImpl<cfg>(binaryData, buffer); \
        break;

    switch (elf::detectArch(buffer)) {
        CFG_ARCH_CASE(ConfigARM);
        CFG_ARCH_CASE(ConfigAarch64);
    }

#undef CFG_ARCH_BASE

    if (error != Error::Success)
        return error;

    m_Type = binaryData.type;
    m_CodeBuffer = binaryData.codeBuffer;
    return Error::Success;
}

std::uintptr_t Base::getSymbolAddress(const std::string &sym) const {
    return 0;
}

std::string binary::getErrorAsString(Error error) {
#define ERROR_CASE(s) \
    case Error::s:    \
        return #s;

    switch (error) {
        ERROR_CASE(Success);
        ERROR_CASE(OpenFailed);
        ERROR_CASE(UnknownArch);
        ERROR_CASE(InvalidObject);
        ERROR_CASE(NoSegments);
        ERROR_CASE(InvalidSegments);
        ERROR_CASE(NoMemory);
        ERROR_CASE(RelocationFailed);
    }

#undef ERROR_CASE

    return {};
}

std::string binary::getTypeAsString(Type type) {
#define TYPE_CASE(s) \
    case Type::s:    \
        return #s;

    switch (type) {
        TYPE_CASE(ARM_LE);
        TYPE_CASE(Aarch64_LE);
    }

#undef TYPE_CASE

    return {};
}