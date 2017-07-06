#include <android/log.h>

#define __libc_fatal(format, arguments...) \
    __android_log_print(ANDROID_LOG_FATAL, "JEMALLOC", format, ##arguments)

static int __attribute__((unused)) pthread_atfork(
    void (*prepare)(void),
    void (*parent)(void),
    void (*child)(void)) {
  return 0;
}
