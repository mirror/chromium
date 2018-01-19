// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crazy_linker_machine_translation.h"

#include "crazy_linker_debug.h"
#include "crazy_linker_globals.h"
#include "crazy_linker_system.h"

#include <dlfcn.h>

namespace crazy {

#if !defined(__i386__)
// Try to detect whether Intel's houdini is installed on this device, but not
// necessarily running for the current process. This probably only works on
// KitKat, and this is based on information grabbed from the Internet, e.g.:
//   http://permalink.gmane.org/gmane.comp.handhelds.android.x86/26293
//   https://sourceforge.net/p/android-x86/vendor_intel_houdini/ci/0de89333e8de2081c1f0b2461f7dbaa29719b554/tree/src/houdini_hook.cpp
//
static bool DetectIntelHoudiniSystemFiles() {
  return PathExists("/system/lib/libhoudini.so") ||
         PathExists("/system/lib/libdvm_houdini.so");
}

// Return true if ART's native bridge is enabled (which means that
// machine translation is enabled on Android L+).
//
// ART, introduced with Lollipop (API level 22), provides a
// framework-sanctioned way to implement machine translation through
// the NativeBridge library (libnativebridge.so), exposing a method called
// android::NativeBridgeInitialized() that can be called at runtime.
//
// NOTE: Before Android L, the library didn't exist, and this function will
//       always return false.
//
// NOTE: On Android N+, linker namespaces prevent this code from opening
//       non-NDK libraries, and thus this will always return false.
//
static bool AndroidNativeBridgeIsInitialized() {
  // NOTE: Code inspired from base::SafeToUseSignalHandler()
  void* lib_handle = ::dlopen("libnativebridge.so", RTLD_NOW);
  if (!lib_handle) {
    // Could not find (KitKat) or open (Nougat+) the library.
    LOG("%s: Could not find libnativebridge.so: %s", __PRETTY_FUNCTION__,
        ::dlerror());
    return false;
  }
  bool result = false;

  // Find the mangled C++ symbol for android::NativeBridgeInitialized().
  // And call if it available.
  auto* initialized_func = reinterpret_cast<bool (*)()>(
      ::dlsym(lib_handle, "_ZN7android23NativeBridgeInitializedEv"));

  if (initialized_func) {
    result = (*initialized_func)();
    LOG("%s: Native bridge initalized: %s", __PRETTY_FUNCTION__,
        result ? "true" : "false");
  } else {
    LOG("%s: Could not find android::NativeBridgeInitialized() symbol",
        __PRETTY_FUNCTION__);
  }
  ::dlclose(lib_handle);
  return result;
}
#endif  // !__i386__

bool MaybeRunningUnderMachineTranslation() {
#if defined(__i386__)
  // This is an x86 crazy linker binary, very probably running on an x86
  // device, so assume there is no machine translation going on.
  return false;
#else   // !__i386__
  const int kApiLevelLollipop = 22;
  const int kApiLevelNougat = 24;
  int api_level = *Globals::GetSDKBuildVersion();
  if (api_level == 0) {
    LOG("%s: ERROR: Unknown SDK API level, crazy_set_sdk_build_version() "
        "call missing",
        __PRETTY_FUNCTION__);
  }
  LOG("%s: API level=%d", __PRETTY_FUNCTION__, api_level);
  if (api_level >= kApiLevelLollipop && api_level < kApiLevelNougat) {
    return AndroidNativeBridgeIsInitialized();
  }
  // Special note for Nougat: AndroidNativeBridgeIsInitialized() cannot work
  // on Nougat, and Intel doesn't support mobile Atom-based chipsets since
  // 2016, it is unlikely, but not impossible, to see Android x86 devices
  // running on Nougat in the wild. As a special best effort, try to detect
  // an Houdini installation, as for KitKat and older releases. Hopefully
  // this should take care of Android-x86 and other similar rare devices,
  // but this has not been tested so far.
  return DetectIntelHoudiniSystemFiles();
#endif  // !__i386__
}

}  // namespace crazy
