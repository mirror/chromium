#include "crazy_linker_native_bridge.h"

#include "crazy_linker_debug.h"
#include "crazy_linker_system.h"

#include <dlfcn.h>

namespace crazy {

bool AndroidNativeBridgeIsInitialized() {
  // TECHNICAL NOTE: In Lollipop, the Android runtime was switched from
  // Dalvik to ART, the later providing a framework-sanctioned way to
  // support runtime machine code translation through the " NativeBridge"
  // library. This makes it relatively easy to detect at runtime by calling
  // the android::NativeBridgeInitialized() function, from the
  // libnativebridge.so system library.
  //
  // Before that (e.g. KitKat), runtime translation was implemented by
  // performing ad-hoc framework modifications and is much harder to detect
  // reliably.

  // NOTE: Code inspired from base::SafeToUseSignalHandler()

  void* lib_handle = ::dlopen("libnativebridge.so", RTLD_NOW);
  if (!lib_handle) {
    // Could not find the library. Maybe Android K?
    // On Android N+, linker namespaces prevent dlopen() to
    // see non-NDK libraries, however there are no native-bridge
    // enabled devices on these releases.
    LOG("%s: Could not find libnativebridge.so: %s", __FUNCTION__, ::dlerror());
    return false;
  }
  bool result = false;

  // Find the mangled C++ symbol for android::NativeBridgeInitialized().
  // And call if it available.
  auto* initialized_func = reinterpret_cast<bool (*)()>(
      ::dlsym(lib_handle, "_ZN7android23NativeBridgeInitializedEv"));

  if (initialized_func) {
    result = (*initialized_func)();
    LOG("%s: Native bridge initalized: %s", __FUNCTION__,
        result ? "true" : "false");
  } else {
    LOG("%s: Could not find android::NativeBridgeInitialized() symbol",
        __FUNCTION__);
  }
  ::dlclose(lib_handle);
  return result;
}

}  // namespace crazy
