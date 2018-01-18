#ifndef CRAZY_LINKER_NATIVE_BRIDGE_H
#define CRAZY_LINKER_NATIVE_BRIDGE_H

namespace crazy {

// Returns true if the Android ART native bridge
// is initialized and running. This corresponds to
// runtime CPU architecture translation (e.g. Intel's
// houdini, which runs ARM binaries on x86 devices).
//
// NOTE: This will always return false for devices that don't have ART
// (e.g. anything before Android Lollipop).
bool AndroidNativeBridgeIsInitialized();

}  // namespace crazy

#endif  // CRAZY_LINKER_NATIVE_BRIDGE_H
