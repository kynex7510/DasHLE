#include "DasHLE/Emulated/JNI.h"

#define REGISTER_FUNC_32(name) bridge->registerFunction<dashle::BITS_32, EmuJNI_##name>("EmuJNI_"#name)
#define REGISTER_FUNC_64(name) bridge->registerFunction<dashle::BITS_64, EmuJNI_##name>("EmuJNI_"#name)
#define REGISTER_FUNC_ANY(name) bridge->registerFunction<dashle::BITS_ANY, EmuJNI_##name>("EmuJNI_"#name)

using namespace dashle;
using namespace dashle::emulated::jni;

// JavaVM methods

static jint EmuJNI_JavaVM_GetEnv(uaddr vm, uaddr env, jint version, bool is64Bits) {
    auto ctx = JNIContext::getInstance();
    uaddr virtualEnv = ctx->virtualEnv();
    if (ctx->virtualVM() != vm)
        virtualEnv = 0u;

    DASHLE_ASSERT_WRAPPER_CONST(block, ctx->getMem()->blockFromVAddr(env));
    DASHLE_ASSERT_WRAPPER_CONST(nativeAddr, host::memory::virtualToHost(*block, env));
    if (is64Bits) {
        *reinterpret_cast<u64*>(nativeAddr) = virtualEnv;
    } else {
        *reinterpret_cast<u32*>(nativeAddr) = virtualEnv;
    }
    return virtualEnv ? binary::jni::OK : binary::jni::DETACHED;
}

static jint EmuJNI_JavaVM_GetEnv32(u32 vm, u32 env, jint version) {
    return EmuJNI_JavaVM_GetEnv(vm, env, version, false);
}

static jint EmuJNI_JavaVM_GetEnv64(u64 vm, u64 env, jint version)  {
    return EmuJNI_JavaVM_GetEnv(vm, env, version, true);
}

// JNIContext

void JNIContext::populateBridge(host::bridge::Bridge* bridge) {
    // JavaVM methods.
    REGISTER_FUNC_32(JavaVM_GetEnv32);
    REGISTER_FUNC_64(JavaVM_GetEnv64);
}