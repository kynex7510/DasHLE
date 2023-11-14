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

struct RelocationInfo {
    uaddr addr;   // Where to apply the relocation.
    u32 type;     // Relocation type.
    uaddr symbol; // Symbol value.
    soff addend;  // Constant addend.
};

// Relocs

static Expected<void> handleSingle(const RelocationContext& ctx, const RelocationInfo& info) {
    if (info.type == R_ARM_RELATIVE) {
        if (info.addend) {
            *reinterpret_cast<elf::Config::AddrType*>(info.addr) = ctx.virtualBase + info.addend;
        } else {
            *reinterpret_cast<elf::Config::AddrType*>(info.addr) += ctx.virtualBase;
        }

        return EXPECTED_VOID;
    }

    if (info.type == R_ARM_ABS32 || info.type == R_ARM_GLOB_DAT || info.type == R_ARM_JUMP_SLOT) {
        DASHLE_ASSERT(info.symbol);
        *reinterpret_cast<elf::Config::AddrType*>(info.addr) = info.symbol + info.addend;
        return EXPECTED_VOID;
    }

    return Unexpected(Error::InvalidRelocation);
}

static Expected<void> handleRelArray(const RelocationContext& ctx, const RelType* relArray, usize size) {
    for (auto i = 0u; i < size; ++i) {
        const auto rel = &relArray[i];
        DASHLE_TRY_EXPECTED_CONST(addr, ctx.resolver->virtualToHost(ctx.virtualBase + rel->r_offset));
        DASHLE_TRY_EXPECTED_CONST(symbolName, elf::getSymbolName(ctx.header, rel->symbolIndex()));

        uaddr symbol = 0u;
        if (!symbolName.empty()) {
            const auto ret = ctx.resolver->resolveSymbol(symbolName);
            if (!ret)
                return Unexpected(ret.error());
            symbol = ret.value();
        }

        const auto info = RelocationInfo {
            .addr = addr,
            .type = rel->type(),
            .symbol = symbol,
            .addend = 0u  
        };
        
        DASHLE_TRY_EXPECTED_VOID(handleSingle(ctx, info));
    }

    return EXPECTED_VOID;
}

static Expected<void> handleRelaArray(const RelocationContext& ctx, const RelaType* relaArray, usize size) {
    for (auto i = 0u; i < size; ++i) {
        const auto rela = &relaArray[i];
        DASHLE_TRY_EXPECTED_CONST(addr, ctx.resolver->virtualToHost(ctx.virtualBase + rela->r_offset));
        DASHLE_TRY_EXPECTED_CONST(symbolName, elf::getSymbolName(ctx.header, rela->symbolIndex()));
        DASHLE_TRY_EXPECTED_CONST(symbol, ctx.resolver->resolveSymbol(symbolName));
        const auto info = RelocationInfo {
            .addr = addr,
            .type = rela->type(),
            .symbol = symbol,
            .addend = rela->r_addend
        };

        DASHLE_TRY_EXPECTED_VOID(handleSingle(ctx, info));
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