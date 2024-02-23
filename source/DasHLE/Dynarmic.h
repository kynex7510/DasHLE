#ifndef _DASHLE_DYNARMIC_H
#define _DASHLE_DYNARMIC_H

#include "dynarmic/interface/exclusive_monitor.h"
#include "dynarmic/interface/A32/a32.h"
#include "dynarmic/frontend/A32/a32_types.h"
#include "dynarmic/frontend/A32/a32_ir_emitter.h"
#include "dynarmic/interface/A64/a64.h"
#include "dynarmic/frontend/A64/a64_types.h"
#include "dynarmic/frontend/A64/a64_ir_emitter.h"

namespace dashle {
namespace dynarmic = Dynarmic;
namespace dynarmic32 = Dynarmic::A32;
namespace dynarmic64 = Dynarmic::A64;
namespace dynarmic_ir = Dynarmic::IR;
} // namespace dashle

#endif /* _DASHLE_DYNARMIC_H */