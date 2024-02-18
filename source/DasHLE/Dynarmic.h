#ifndef _DASHLE_DYNARMIC_H
#define _DASHLE_DYNARMIC_H

#include "dynarmic/interface/exclusive_monitor.h"
#include "dynarmic/interface/A32/a32.h"
#include "dynarmic/interface/A64/a64.h"

#if defined(DASHLE_HAS_DYNARMIC_IR)
#include "dynarmic/frontend/A32/a32_ir_emitter.h"
#include "dynarmic/frontend/A64/a64_ir_emitter.h"
#endif // DASHLE_HAS_DYNARMIC_IR

namespace dashle {
namespace dynarmic = Dynarmic;
namespace dynarmic32 = Dynarmic::A32;
namespace dynarmic64 = Dynarmic::A64;

#if defined(DASHLE_HAS_DYNARMIC_IR)
namespace dynarmic_ir = Dynarmic::IR;
#endif // DASHLE_HAS_DYNARMIC_IR

} // namespace dashle

// TODO: this is only here for linting and should be removed once dynarmic64 gets support for code hooks.
namespace Dynarmic::A64 {
    class IREmitter;
} // namespace Dynarmic::A64

#endif /* _DASHLE_DYNARMIC_H */