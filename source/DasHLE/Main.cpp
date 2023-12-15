#define DASHLE_HAS_DYNARMIC_IR

#include "DasHLE/Guest/VM.h"
#include "DasHLE/Guest/ARM.h"

using namespace dashle;
namespace regs = dashle::guest::arm::regs;

#define DECLARE_EMITTER(name) \
    static void name##Emit(host::interop::IREmitterWrapper emitter) { emitCallToFunc(emitter, name##Wrapper); }

#define DECLARE_CALLBACK(name) \
    DECLARE_EMITTER(name);     \
    static void name##Wrapper(u64 dummy0, u64 dummy1, u64 dummy2)

#define REGISTER_CALLBACK(name)                                                             \
    {                                                                                       \
        DASHLE_ASSERT_WRAPPER_CONST(vaddr, m_Interop->registerEmitterCallback(name##Emit)); \
        m_Symbols.insert({ (#name), vaddr });                                               \
    }

constexpr usize MEM_4GB = static_cast<usize>(1u) << 32;
constexpr const char BINARY_PATH[] = "/home/user/Documents/repos/DasHLE/app/lib/armeabi-v7a/libcocos2dcpp.so";

// MyContext

class MyContext : public host::interop::SymResolver {
    std::shared_ptr<host::memory::MemoryManager> m_Mem;
    std::shared_ptr<host::interop::InteropHandler> m_Interop;
    std::unique_ptr<guest::GuestVM> m_VM;
    std::unordered_map<std::string, uaddr> m_Symbols;

    static void emitCallToFunc(host::interop::IREmitterWrapper emitter, void(*fn)(u64, u64, u64)) {
        // BUG: https://github.com/merryhime/dynarmic/issues/767
        const auto dummy = dynarmic_ir::U64(dynarmic_ir::Value(0ul));
        emitter->CallHostFunction(fn, dummy, dummy, dummy);
        emitter->BXWritePC(emitter->GetRegister(dynarmic32::Reg(regs::LR)));
        emitter->SetTerm(dynarmic_ir::Term::ReturnToDispatch{});
    }

    // Callbacks

    DECLARE_CALLBACK(pthread_once) {
        DASHLE_LOG_LINE("Hello from pthread_once");
        auto vm = getInstance()->m_VM.get();
        const auto ctrl = vm->getRegister(regs::R0);
        const auto routine = vm->getRegister(regs::R1);
        const auto lr = vm->getRegister(regs::LR);
        DASHLE_LOG_LINE("pthread_once called with ctrl = 0x{:X}", ctrl);
        vm->setRegister(regs::R0, 88);
    }

    DECLARE_CALLBACK(pthread_key_create) {
        DASHLE_LOG_LINE("Hello from pthread_key_create");
        getInstance()->m_VM->setRegister(regs::R0, 88);
    }

    DECLARE_CALLBACK(memcpy) {
        DASHLE_LOG_LINE("Hello from memcpy");
        auto vm = getInstance()->m_VM.get();
        const auto destVAddr = vm->getRegister(regs::R0);
        DASHLE_ASSERT_WRAPPER_CONST(destAddr, vm->virtualToHost(destVAddr));
        DASHLE_ASSERT_WRAPPER_CONST(srcAddr, vm->virtualToHost(vm->getRegister(regs::R1)));
        const auto size = vm->getRegister(regs::R2);
        memcpy(reinterpret_cast<void*>(destAddr), reinterpret_cast<void*>(srcAddr), size);
        vm->setRegister(regs::R0, destVAddr);
    }

    DECLARE_CALLBACK(memset) {
        DASHLE_LOG_LINE("Hello from memset");
        auto vm = getInstance()->m_VM.get();
        const auto destVAddr = vm->getRegister(regs::R0);
        DASHLE_ASSERT_WRAPPER_CONST(destAddr, vm->virtualToHost(destVAddr));
        const auto value = vm->getRegister(regs::R1);
        const auto size = vm->getRegister(regs::R2);
        memset(reinterpret_cast<void*>(destAddr), value, size);
        vm->setRegister(regs::R0, destVAddr);
    }

    DECLARE_CALLBACK(strcmp) {
        DASHLE_LOG_LINE("Hello from strcmp");
        auto vm = getInstance()->m_VM.get();
        DASHLE_ASSERT_WRAPPER_CONST(s1, vm->virtualToHost(vm->getRegister(regs::R0)));
        DASHLE_ASSERT_WRAPPER_CONST(s2, vm->virtualToHost(vm->getRegister(regs::R1)));
        vm->setRegister(regs::R0, strcmp(reinterpret_cast<char*>(s1), reinterpret_cast<char*>(s2)));
    }

    DECLARE_CALLBACK(wctob) {
        DASHLE_LOG_LINE("Hello from wctob");
        auto vm = getInstance()->m_VM.get();
        vm->setRegister(regs::R0, std::wctob(vm->getRegister(regs::R0)));
    }

    DECLARE_CALLBACK(btowc) {
        DASHLE_LOG_LINE("Hello from btowc");
        auto vm = getInstance()->m_VM.get();
        vm->setRegister(regs::R0, std::btowc(vm->getRegister(regs::R0)));
    }

    DECLARE_CALLBACK(wctype) {
        DASHLE_LOG_LINE("Hello from wctype");
        auto vm = getInstance()->m_VM.get();
        DASHLE_ASSERT_WRAPPER_CONST(ptr, vm->virtualToHost(vm->getRegister(regs::R0)));
        vm->setRegister(regs::R0, std::wctype(reinterpret_cast<char*>(ptr)));
    }

    DECLARE_CALLBACK(__cxa_atexit) { DASHLE_LOG_LINE("Hello from __cxa_atexit"); }

    DECLARE_CALLBACK(strlen) {
        DASHLE_LOG_LINE("Hello from strlen");
        auto vm = getInstance()->m_VM.get();
        DASHLE_ASSERT_WRAPPER_CONST(ptr, vm->virtualToHost(vm->getRegister(regs::R0)));
        vm->setRegister(regs::R0, std::strlen(reinterpret_cast<char*>(ptr)));
    }

    DECLARE_CALLBACK(malloc) {
        DASHLE_LOG_LINE("Hello from malloc");
        auto vm = getInstance()->m_VM.get();
        auto mem = getInstance()->m_Mem.get();
        const auto size = vm->getRegister(regs::R0);
        DASHLE_ASSERT_WRAPPER_CONST(block, mem->allocate({.size = size}));
        vm->setRegister(regs::R0, block->virtualBase);
    }

    // Context stuff.

    void registerAllCallbacks() {
        REGISTER_CALLBACK(pthread_once);
        //REGISTER_CALLBACK(pthread_key_create);
        REGISTER_CALLBACK(memcpy);
        //REGISTER_CALLBACK(memset);
        //REGISTER_CALLBACK(strcmp);
        //REGISTER_CALLBACK(wctob);
        //REGISTER_CALLBACK(btowc);
        //REGISTER_CALLBACK(wctype);
        //REGISTER_CALLBACK(__cxa_atexit)
        //REGISTER_CALLBACK(strlen);
        //REGISTER_CALLBACK(malloc);
    }

    MyContext() {
        m_Mem = std::make_shared<host::memory::MemoryManager>(
            std::make_unique<host::memory::HostAllocator>(),
            MEM_4GB - guest::arm::PAGE_SIZE,
            32,
            guest::arm::PAGE_SIZE);

        m_Interop = std::make_shared<host::interop::InteropHandler>(m_Mem, 11);
        registerAllCallbacks();

        DASHLE_ASSERT_WRAPPER(vm, dashle::guest::makeVMFromFile(BINARY_PATH, dashle::guest::VMArgs {
            .mem = m_Mem,
            .interop = m_Interop,
            .resolver = this,
        }));
        m_VM = std::move(vm);
    }

    Expected<uaddr> resolve(const std::string& symbol) const override {
        // Look for functions.
        if (auto it = m_Symbols.find(symbol); it != m_Symbols.end())
            return it->second;

        //return Unexpected(Error::NotFound);
        return 0u; // Ignore, for now.
    }

public:
    static MyContext* getInstance() {
        static MyContext ctx;
        return &ctx;
    }

    void run() { m_VM->runInitializers(); }
    void dump() { m_VM->dump(); }
};

/*


using JNI_OnLoad_t = jni::jint(*)(jni::JavaVM* vm, void* reserved);
using Cocos2dxHelper_nativeSetApkPath_t = void(*)(jni::JNIEnv* env, jni::jobject thiz, jni::jstring apkPath);
using Cocos2dxRenderer_nativeInit_t = void(*)(jni::JNIEnv* env, jni::jobject thiz, jni::jint w, jni::jint h);

DASHLE_LOG_LINE("Executing JNI_OnLoad...");
vm->setRegister(regs::R0, 0); // JavaVM*
vm->setRegister(regs::R1, 0); // void*
vm->execute(0x1E4930 + 0x1000 + 0x1);
DASHLE_ASSERT(vm->getRegister(regs::R0) == 0x00010004);

DASHLE_LOG_LINE("Executing Java_org_cocos2dx_lib_Cocos2dxHelper_nativeSetApkPath...");
vm->setRegister(regs::R0, 0); // JNIEnv*
vm->setRegister(regs::R1, 0); // jobject
vm->setRegister(regs::R2, 0); // jstring
vm->execute(0x3D4C4E + 0x1000 + 0x1);

DASHLE_LOG_LINE("Executing Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeInit...");
vm->setRegister(regs::R0, 0); //JNIEnv*
vm->setRegister(regs::R1, 0); // jobject
vm->setRegister(regs::R2, 800); // jint
vm->setRegister(regs::R3, 600); // jint
vm->execute(0x1E4940 + 0x1000 + 0x1);

*/

int main() {
    auto ctx = MyContext::getInstance();
    ctx->run();
    ctx->dump();
    return 0;
}