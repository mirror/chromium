
#include <jni.h>

#define JNI_GENERATOR_EXPORT \
  extern "C" __attribute__((visibility("default")))

JNI_GENERATOR_EXPORT int
    Java_org_chromium_webapk_lib_runtime_1library_WebApkServiceImpl_nativeCreateSocket(int family, int type, int protocol);

