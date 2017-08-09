
#include "chrome/android/webapk/libs/runtime_library/native.h"
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include <jni.h>
#include <string.h>
#include <android/log.h>


JNI_GENERATOR_EXPORT int
    Java_org_chromium_webapk_lib_runtime_1library_WebApkServiceImpl_nativeCreateSocket(int family, int type, int protocol) {
      //family = PF_INET;
      //type = SOCK_STREAM;
   __android_log_print(ANDROID_LOG_DEBUG, "Yaron webapk natieve", "%d %d %d", family, type, protocol);
   int socket = ::socket(PF_INET, SOCK_STREAM, 0);
   __android_log_print(ANDROID_LOG_DEBUG, "Yaron webapk natieve", "%d", socket);
   return socket;
}

