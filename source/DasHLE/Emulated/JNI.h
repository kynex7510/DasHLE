#ifndef _DASHLE_EMULATED_JNI_H
#define _DASHLE_EMULATED_JNI_H

#include "DasHLE/Host/Memory.h"
#include "DasHLE/Host/Bridge.h"
#include "DasHLE/Binary/JNIDefs.h"

#include <memory>

namespace dashle::emulated::jni {

using namespace binary::jni;

class JNIContext final {
    std::shared_ptr<host::memory::MemoryManager> m_Mem;
    JavaVM* m_NativeVM;
    uaddr m_VirtualVM;
    JNIEnv* m_NativeEnv;
    uaddr m_VirtualEnv;

    JNIContext() {}

public:
    static JNIContext* getInstance() {
        static JNIContext ctx;
        return &ctx;
    }

    static void populateBridge(host::bridge::Bridge* bridge);

    void setMem(std::shared_ptr<host::memory::MemoryManager> mem) { m_Mem = mem; }

    host::memory::MemoryManager* getMem() {
        DASHLE_ASSERT(m_Mem);
        return m_Mem.get();
    }

    JavaVM* nativeVM() const { return m_NativeVM; }
    uaddr virtualVM() const { return m_VirtualVM; }
    JNIEnv* nativeEnv() const { return m_NativeEnv; }
    uaddr virtualEnv() const { return m_VirtualEnv; }
};

jint JNI_GetDefaultJavaVMInitArgs(void *vm_args);
jint JNI_GetCreatedJavaVMs(JavaVM **vmBuf, jsize bufLen, jsize *nVMs);
jint JNI_CreateJavaVM(JavaVM **p_vm, void **p_env, void *vm_args);

} // namespace dashle::emulated

#endif /* _DASHLE_EMULATED_JNI_H */