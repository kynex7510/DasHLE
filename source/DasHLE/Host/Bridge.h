#ifndef _DASHLE_HOST_BRIDGE_H
#define _DASHLE_HOST_BRIDGE_H

#include "DasHLE/Dynarmic.h"
#include "DasHLE/Host/Memory.h"

#include <unordered_map>
#include <variant>

namespace dashle::host::bridge {

class Bridge final {
    class Emitter final {
        using Emitter32 = void(*)(dynarmic32::IREmitter*);
        using Emitter64 = void(*)(dynarmic64::IREmitter*);

        std::variant<Emitter32, Emitter64> m_Emitter;

    public:
        Emitter() {}
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
        ir->BXWritePC(ir->GetRegister(dynarmic32::Reg::LR));
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
    Bridge(std::shared_ptr<host::memory::MemoryManager> mem, usize bitness);
    ~Bridge();

    // Register a host function to be called from the Jit.
    template <usize BITS, auto FN>
    Expected<void> registerFunction(const std::string& symbol) {
        if constexpr (BITS & dashle::BITS_32) {
            if (m_Bitness & dashle::BITS_32) {
                DASHLE_TRY_EXPECTED_VOID(registerFunctionImpl(symbol, &emitCall32<FN>));
            }
        }

        if constexpr (BITS & dashle::BITS_64) {
            if (m_Bitness & dashle::BITS_64) {
                DASHLE_TRY_EXPECTED_VOID(registerFunctionImpl(symbol, &emitCall64<FN>));
            }
        }

        return EXPECTED_VOID;
    }

    // Register a virtual pointer to a host variable to be used from the Jit.
    Expected<void> registerVariable(const std::string& symbol, uaddr vaddr);

    // Build the table used by the Jit to call the correct functions.
    Expected<void> buildIFT();
    bool hasBuiltIFT() const { return m_IFTBase != 0u; }

    bool hasSymbol(const std::string& symbol) const {
        return m_FuncEntries.contains(symbol) || m_VarEntries.contains(symbol);
    }

    // Return the virtual address used by the Jit to access a function/variable.
    Expected<uaddr> addressForSymbol(const std::string& symbol);

    // Used by the Jit to generate the code calling into host functions. 
    Expected<void> emitCall(uaddr vaddr, dynarmic32::IREmitter* ir);
    Expected<void> emitCall(uaddr vaddr, dynarmic64::IREmitter* ir);
};

} // namespace dashle::host::bridge

#endif /* _DASHLE_HOST_BRIDGE_H */