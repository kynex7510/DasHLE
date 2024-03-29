#include "DasHLE/Host/Bridge.h"

using namespace dashle;
using namespace dashle::host::bridge;

// To the Jit an IFT entry is just a location to some instruction.
constexpr static usize ENTRY_SIZE = 4u;

// Special address used when no functions are set.
constexpr static auto IFT_NO_FUNC = static_cast<usize>(-1);

// Bridge

Expected<void> Bridge::registerFunctionImpl(const std::string& symbol, Emitter emitter) {
    if (hasBuiltIFT())
        return Unexpected(Error::InvalidOperation);

    if (hasSymbol(symbol))
        return Unexpected(Error::Duplicate);

    const auto vaddr = m_FuncEntries.size() * ENTRY_SIZE;
    m_FuncEntries[symbol] = vaddr;
    m_Emitters[vaddr] = emitter;
    return EXPECTED_VOID;
}

template <typename IR>
requires (OneOf<IR, dynarmic32::IREmitter*, dynarmic64::IREmitter*>)
Expected<void> Bridge::invokeEmitterImpl(uaddr vaddr, IR ir) {
    if (!hasBuiltIFT())
        return Unexpected(Error::InvalidOperation);

    if (auto it = m_Emitters.find(vaddr); it != m_Emitters.end()) {
        it->second.invoke(ir);
        return EXPECTED_VOID;
    }

    return Unexpected(Error::NotFound);
}

Bridge::Bridge(std::shared_ptr<host::memory::MemoryManager> mem, usize bitness)
    : m_Mem(mem), m_Bitness(bitness) {
    DASHLE_ASSERT(m_Bitness == dashle::BITS_32 || m_Bitness == dashle::BITS_64);
}

Bridge::~Bridge() {
    if (hasBuiltIFT() && m_IFTBase != IFT_NO_FUNC) {
        DASHLE_ASSERT(m_Mem->free(m_IFTBase));
    }
}

Expected<void> Bridge::registerVariable(const std::string& symbol, uaddr vaddr) {
    if (hasBuiltIFT())
        return Unexpected(Error::InvalidOperation);

    if (hasSymbol(symbol))
        return Unexpected(Error::Duplicate);

    if (!m_Mem->blockFromVAddr(vaddr).has_value())
        return Unexpected(Error::InvalidAddress);

    m_VarEntries[symbol] = vaddr;
    return EXPECTED_VOID;
}

Expected<void> Bridge::buildIFT() {
    if (hasBuiltIFT())
        return Unexpected(Error::InvalidOperation);

    // Handle the case when we dont have functions.
    if (m_FuncEntries.empty()) {
        m_IFTBase = IFT_NO_FUNC;
        return EXPECTED_VOID;
    }

    // Allocate IFT: a table of reserved addresses which act as pseudo addresses for imported functions.
    // You can imagine as if the whole function is contained within its entry address, the Jit will jump to the
    // correct function when executing from one of the entries. 
    DASHLE_TRY_EXPECTED_CONST(block, m_Mem->allocate({
        .size = m_FuncEntries.size() * ENTRY_SIZE,
        .alignment = ENTRY_SIZE,
        .flags = 0u,
    }));

    m_IFTBase = block->virtualBase;

    // Rebase addresses.
    for (auto &[_, value] : m_FuncEntries)
        value += m_IFTBase;

    EmitterMap emitters;
    while (m_Emitters.begin() != m_Emitters.end()) {
        auto node = m_Emitters.extract(m_Emitters.begin());
        emitters.insert({ m_IFTBase + node.key(), node.mapped() });
    }
    m_Emitters = std::move(emitters);

    return EXPECTED_VOID;
}

Expected<uaddr> Bridge::addressForSymbol(const std::string& symbol) {
    if (!hasBuiltIFT())
        return Unexpected(Error::InvalidOperation);

    if (auto it = m_FuncEntries.find(symbol); it != m_FuncEntries.end())
        return it->second;

    if (auto it = m_VarEntries.find(symbol); it != m_VarEntries.end())
        return it->second;

    return Unexpected(Error::NotFound);
}

Expected<void> Bridge::emitCall(uaddr vaddr, dynarmic32::IREmitter* ir) {
    return invokeEmitterImpl(vaddr, ir);
}

Expected<void> Bridge::emitCall(uaddr vaddr, dynarmic64::IREmitter* ir) {
    return invokeEmitterImpl(vaddr, ir);
}