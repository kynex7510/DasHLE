#include "DasHLE/Emu/ARM/Relocs.h"

using namespace dashle;
using namespace dashle::emu::arm;

constexpr static auto R_ARM_ABS32 = 2;
constexpr static auto R_ARM_GLOB_DAT = 21;
constexpr static auto R_ARM_JUMP_SLOT = 22;
constexpr static auto R_ARM_RELATIVE = 23;

using HeaderType = elf::Config::HeaderType;
using RelType = elf::Config::RelType;
using RelaType = elf::Config::RelaType;
using AddrType = elf::Config::AddrType;

constexpr static bool isRelativeReloc(usize type) {
    return type == R_ARM_RELATIVE;
}

constexpr static bool isSymbolReloc(usize type) {
    return type == R_ARM_ABS32 || type == R_ARM_GLOB_DAT || type == R_ARM_JUMP_SLOT;
}

// Relocs

static Expected<uaddr> resolveSymbol(const RelocationContext& ctx, elf::Config::WordType symbolIndex) {
    DASHLE_TRY_EXPECTED_CONST(symbolName, elf::getSymbolName(ctx.header, symbolIndex));
    return ctx.resolver->resolveSymbol(symbolName);
}

static Expected<void> handleRelArray(const RelocationContext& ctx, const RelType* relArray, usize size) {
    for (auto i = 0u; i < size; ++i) {
        const auto rel = &relArray[i];
        DASHLE_TRY_EXPECTED_CONST(patchAddr, ctx.resolver->virtualToHost(ctx.virtualBase + rel->r_offset));

        // Handle base relative relocation.
        if (isRelativeReloc(rel->type())) {
            *reinterpret_cast<elf::Config::AddrType*>(patchAddr) += ctx.virtualBase;
            continue;
        }

        // Handle symbol relocation.
        if (isSymbolReloc(rel->type())) {
            DASHLE_TRY_EXPECTED_CONST(value, resolveSymbol(ctx, rel->symbolIndex()));
            *reinterpret_cast<elf::Config::AddrType*>(patchAddr) = value;
            continue;
        }

        return Unexpected(Error::InvalidRelocation);
    }

    return EXPECTED_VOID;
}

static Expected<void> handleRelaArray(const RelocationContext& ctx, const RelaType* relaArray, usize size) {
    for (auto i = 0u; i < size; ++i) {
        const auto rela = &relaArray[i];
        DASHLE_TRY_EXPECTED_CONST(patchAddr, ctx.resolver->virtualToHost(ctx.virtualBase + rela->r_offset));

        // Handle base relative relocation.
        if (isRelativeReloc(rela->type())) {
            *reinterpret_cast<elf::Config::AddrType*>(patchAddr) = ctx.virtualBase + rela->r_addend;
            continue;
        }

        // Handle symbol relocation.
        if (isSymbolReloc(rela->type())) {
            DASHLE_TRY_EXPECTED_CONST(value, resolveSymbol(ctx, rela->symbolIndex()));
            // We ignore the addend.
            *reinterpret_cast<elf::Config::AddrType*>(patchAddr) = value;
            continue;
        }

        return Unexpected(Error::InvalidRelocation);
    }

    return EXPECTED_VOID;
}

static Expected<void> handleRel(const RelocationContext& ctx) {
    const auto relEntry = elf::getDynEntry(ctx.header, elf::DT_REL);
    const auto relEntrySize = elf::getDynEntry(ctx.header, elf::DT_RELSZ);
    const auto relEntryEnt = elf::getDynEntry(ctx.header, elf::DT_RELENT);

    if (relEntry && relEntrySize && relEntryEnt) {
        const auto base = reinterpret_cast<uaddr>(ctx.header);
        const auto relArray = reinterpret_cast<RelType*>(base + relEntry.value()->d_un.d_ptr);
        const usize size = relEntrySize.value()->d_un.d_val / relEntryEnt.value()->d_un.d_val;
        return handleRelArray(ctx, relArray, size);
    }

    return EXPECTED_VOID;
}

static Expected<void> handleRela(const RelocationContext& ctx) {
    const auto relaEntry = elf::getDynEntry(ctx.header, elf::DT_RELA);
    const auto relaEntrySize = elf::getDynEntry(ctx.header, elf::DT_RELASZ);
    const auto relaEntryEnt = elf::getDynEntry(ctx.header, elf::DT_RELAENT);

    if (relaEntry && relaEntrySize && relaEntryEnt) {
        const auto base = reinterpret_cast<uaddr>(ctx.header);
        const auto relaArray = reinterpret_cast<RelaType*>(base + relaEntry.value()->d_un.d_ptr);
        const usize size = relaEntrySize.value()->d_un.d_val / relaEntryEnt.value()->d_un.d_val;
        return handleRelaArray(ctx, relaArray, size);
    }

    return EXPECTED_VOID;
}

static Expected<void> handleJmprel(const RelocationContext& ctx) {
    const auto jmprelEntry = elf::getDynEntry(ctx.header, elf::DT_JMPREL);
    const auto jmprelEntrySize = elf::getDynEntry(ctx.header, elf::DT_PLTRELSZ);
    const auto jmprelEntryType = elf::getDynEntry(ctx.header, elf::DT_PLTREL);

    if (!jmprelEntry || !jmprelEntrySize || !jmprelEntryType)
        return EXPECTED_VOID;

    const auto base = reinterpret_cast<uaddr>(ctx.header);

    if (jmprelEntryType.value()->d_un.d_val == elf::DT_REL) {
        const auto jmprelArray = reinterpret_cast<RelType*>(base + jmprelEntry.value()->d_un.d_ptr);
        const usize size = jmprelEntrySize.value()->d_un.d_val / sizeof(RelType);   
        return handleRelArray(ctx, jmprelArray, size);
    }

    if (jmprelEntryType.value()->d_un.d_val == elf::DT_RELA) {
        const auto jmprelArray = reinterpret_cast<RelaType*>(base + jmprelEntry.value()->d_un.d_ptr);
        const usize size = jmprelEntrySize.value()->d_un.d_val / sizeof(RelaType);
        return handleRelaArray(ctx, jmprelArray, size);
    }

    return Unexpected(Error::InvalidRelocation);
}

Expected<void> dashle::emu::arm::handleRelocations(const RelocationContext& ctx) {
    return handleRel(ctx).and_then([&ctx] {
        return handleRela(ctx);
    }).and_then([&ctx] {
        return handleJmprel(ctx);
    });
}