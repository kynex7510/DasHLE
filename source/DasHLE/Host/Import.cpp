#include "DasHLE/Host/Import.h"

using namespace dashle;
using namespace dashle::host::import;

// To the JIT an IFT entry is just a location to some instruction.
constexpr static usize ENTRY_SIZE = 4u;

// ImportHandler

Expected<void> ImportHandler::registerFunctionImpl(const std::string& symbol, Emitter emitter) {
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
Expected<void> ImportHandler::invokeEmitterImpl(uaddr vaddr, IR ir) {
    if (!hasBuiltIAT())
        return Unexpected(Error::InvalidOperation);

    if (auto it = m_Emitters.find(vaddr); it != m_Emitters.end()) {
        it->second.invoke(ir);
        return EXPECTED_VOID;
    }

    return Unexpected(Error::NotFound);
}

ImportHandler::ImportHandler(std::shared_ptr<host::memory::MemoryManager> mem, usize bitness)
    : m_Mem(mem), m_Bitness(bitness) {
    DASHLE_ASSERT(m_Bitness == BITS_32 || m_Bitness == BITS_64);
}

ImportHandler::~ImportHandler() { DASHLE_ASSERT(m_Mem->free(m_IFTBase)); }

Expected<void> ImportHandler::registerVariable(const std::string& symbol, uaddr vaddr) {
    if (hasBuiltIFT())
        return Unexpected(Error::InvalidOperation);

    if (hasSymbol(symbol))
        return Unexpected(Error::Duplicate);

    if (!m_Mem->blockFromVAddr(vaddr).has_value())
        return Unexpected(Error::InvalidAddress);

    m_VarEntries[symbol] = vaddr;
    return EXPECTED_VOID;
}

Expected<void> ImportHandler::buildIFT() {
    // Allocate IFT: this a table of reserved addresses which act as pseudo addresses for imported functions.
    // You can imagine as if the whole function is contained within its entry address, the JIT will jump to the
    // correct function when executing from one of the entries. 
    DASHLE_TRY_EXPECTED_CONST(block, m_Mem->allocate({
        .size = m_FuncEntries.size() * ENTRY_SIZE,
        .alignment = ENTRY_SIZE,
        .flags = 0u,
    }));

    m_IFTBase = block->virtualBase;

    // Rebase addresses.
    SymbolMap funcEntries;
    while (m_FuncEntries.begin() != m_FuncEntries.end()) {
        auto node = m_FuncEntries.extract(m_FuncEntries.begin());
        funcEntries.insert({ node.key(), m_IFTBase + node.mapped() });
    }

    EmitterMap emitters;
    while (m_Emitters.begin() != m_Emitters.end()) {
        auto node = m_Emitters.extract(m_Emitters.begin());
        emitters.insert( { m_IFTBase +  node.key(), node.mapped() });
    }

    m_FuncEntries = std::move(funcEntries);
    m_Emitters = std::move(emitters);

    return EXPECTED_VOID;
}

Expected<uaddr> ImportHandler::addressForSymbol(const std::string& symbol) {
    if (!hasBuiltIFT())
        return Unexpected(Error::InvalidOperation);

    if (auto it = m_FuncEntries.find(symbol); it != m_FuncEntries.end())
        return it->second;

    if (auto it = m_VarEntries.find(symbol); it != m_VarEntries.end())
        return it->second;

    return Unexpected(Error::NotFound);
}

Expected<void> ImportHandler::invokeEmitter(uaddr vaddr, dynarmic32::IREmitter* ir) {
    invokeEmitterImpl(vaddr, ir);
}

Expected<void> ImportHandler::invokeEmitter(uaddr vaddr, dynarmic64::IREmitter* ir) {
    invokeEmitterImpl(vaddr, ir);
}