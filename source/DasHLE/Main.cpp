#include "DasHLE/Guest/Context.h"

#include "DasHLE/Guest/ARM/Context.h"

#include "DasHLE/Binary/JNI.h"

using namespace dashle;
namespace jni = dashle::binary::jni;
namespace regs = dashle::guest::arm::regs;

constexpr const char BINARY_PATH[] = "/home/user/Documents/repos/DasHLE/app/lib/armeabi-v7a/libcocos2dcpp.so";

using JNI_OnLoad_t = jni::jint(*)(jni::JavaVM* vm, void* reserved);
using Cocos2dxHelper_nativeSetApkPath_t = void(*)(jni::JNIEnv* env, jni::jobject thiz, jni::jstring apkPath);
using Cocos2dxRenderer_nativeInit_t = void(*)(jni::JNIEnv* env, jni::jobject thiz, jni::jint w, jni::jint h);

int main() {
    // Create Cpu object.
    auto ret = dashle::guest::createContext(BINARY_PATH);
    if (!ret) {
        DASHLE_LOG_LINE("Could not open binary ({})", ret.error());
        return 1;
    }

    auto cpu = std::move(ret.value());

    DASHLE_LOG_LINE("Executing JNI_OnLoad...");
    cpu->setRegister(regs::R0, 0); // JavaVM*
    cpu->setRegister(regs::R1, 0); // void*
    cpu->execute(0x1E4930 + 0x1000 + 0x1);
    DASHLE_ASSERT(cpu->getRegister(regs::R0) == 0x00010004);

    DASHLE_LOG_LINE("Executing Java_org_cocos2dx_lib_Cocos2dxHelper_nativeSetApkPath...");
    cpu->setRegister(regs::R0, 0); // JNIEnv*
    cpu->setRegister(regs::R1, 0); // jobject
    cpu->setRegister(regs::R2, 0); // jstring
    cpu->execute(0x3D4C4E + 0x1000 + 0x1);

    DASHLE_LOG_LINE("Executing Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeInit...");
    cpu->setRegister(regs::R0, 0); //JNIEnv*
    cpu->setRegister(regs::R1, 0); // jobject
    cpu->setRegister(regs::R2, 800); // jint
    cpu->setRegister(regs::R3, 600); // jint
    cpu->execute(0x1E4940 + 0x1000 + 0x1);

    cpu->dump();
    return 0;
}