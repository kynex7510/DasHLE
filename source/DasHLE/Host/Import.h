#ifndef _DASHLE_HOST_IMPORT_H
#define _DASHLE_HOST_IMPORT_H

#include "DasHLE/Dynarmic.h"
#include "DasHLE/Host/Memory.h"

#include <unordered_map>
#include <variant>

// TODO: this is only here for linting and should be removed once dynarmic64 gets support for code hooks.
namespace Dynarmic::A64 {
    class IREmitter;
} // namespace Dynarmic::A64

namespace dashle::host::import {

class Emitter final {
    using Emitter32 = void(*)(dynarmic32::IREmitter*);
    using Emitter64 = void(*)(dynarmic64::IREmitter*);

    std::variant<Emitter32, Emitter64> m_Emitter;

public:
    Emitter(Emitter32 emitter) : m_Emitter(emitter) {}
    Emitter(Emitter64 emitter) : m_Emitter(emitter) {}

    void invoke(dynarmic32::IREmitter* ir) const {
        DASHLE_ASSERT(std::holds_alternative<Emitter32>(m_Emitter));
        std::get<Emitter32>(m_Emitter)(ir);
    }

    void invoke(dynarmic64::IREmitter* ir) const {
        DASHLE_ASSERT(std::holds_alternative<Emitter64>(m_Emitter));
        std::get<Emitter64>(m_Emitter)(ir);
    }
};

class ImportHandler final {
    using SymbolMap = std::unordered_map<std::string, uaddr>;
    using EmitterMap = std::unordered_map<uaddr, Emitter>;

    std::shared_ptr<host::memory::MemoryManager> m_Mem;
    const usize m_Bitness;
    uaddr m_IFTBase = 0u;
    SymbolMap m_FuncEntries;
    SymbolMap m_VarEntries;
    EmitterMap m_Emitters;

    template <auto FN>
    void emitCall32(dynarmic32::IREmitter* ir) {
        // TODO
        // ir->CallHostFunctionWithGuestABI(FN);
        ir->BXWritePC(ir->GetRegister(dynarmic32::Reg(regs::LR)));
        ir->SetTerm(dynarmic_ir::Term::ReturnToDispatch{});
    }

    template <auto FN>
    void emitCall64(dynarmic64::IREmitter* ir) {
        DASHLE_UNREACHABLE("Unimplemented!");
    }

    Expected<void> registerFunctionImpl(const std::string& symbol, Emitter emitter);

    template <typename IR>
    requires (OneOf<IR, dynarmic32::IREmitter*, dynarmic64::IREmitter*>)
    Expected<void> invokeEmitterImpl(uaddr vaddr, IR ir);

public:
    ImportHandler(std::shared_ptr<host::memory::MemoryManager> mem, usize bitness);
    virtual ~ImportHandler();

    template <usize BITS, auto FN>
    Expected<void> registerFunction(const std::string& symbol) {
        if constexpr (BITS & BITS_32) {
            if (bitness & BITS_32) {
                DASHLE_TRY_EXPECTED_VOID(registerFunctionImpl(symbol, &emitCall32<FN>));
            }
        }

        if constexpr (BITS & BITS_64) {
            if (bitness & BITS_64) {
                DASHLE_TRY_EXPECTED_VOID(registerFunctionImpl(symbol, &emitCall64<FN>));
            }
        }

        return EXPECTED_VOID;        
    }

    Expected<void> registerVariable(const std::string& symbol, uaddr vaddr);

    Expected<void> buildIFT();
    bool hasBuiltIFT() const { return m_IFTBase != 0u; }

    bool hasSymbol(const std::string& symbol) const {
        return m_FuncEntries.contains(symbol) || m_VarEntries.contains(symbol);
    }

    Expected<uaddr> addressForSymbol(const std::string& symbol);
    Expected<void> invokeEmitter(uaddr vaddr, dynarmic32::IREmitter* ir);
    Expected<void> invokeEmitter(uaddr vaddr, dynarmic64::IREmitter* ir);
};

} // namespace dashle::host::import

#endif /* _DASHLE_HOST_IMPORT_H */