// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SharedGpuContext_h
#define SharedGpuContext_h

#include "platform/PlatformExport.h"
#include "platform/graphics/WebGraphicsContext3DProviderWrapper.h"
#include "platform/wtf/ThreadSpecific.h"

#include <memory>

namespace blink {

class WaitableEvent;
class WebGraphicsContext3DProvider;

// SharedGpuContext provides access to a thread-specific GPU context
// that is shared by many callsites throughout the thread.
// When on the main thread, provides access to the same context as
// Platform::createSharedOffscreenGraphicsContext3DProvider
class PLATFORM_EXPORT SharedGpuContext {
 public:
  // May re-create context if context was lost. If |using_software_compositing|
  // is true on return, then the compositor is not using the gpu, even if the
  // result is a valid pointer. Conversely, if it is false but the result is a
  // null pointer, then the context was not able to be created but it is not yet
  // known that the compositor has given up on using gpu.
  static WeakPtr<WebGraphicsContext3DProviderWrapper> ContextProviderWrapper(
      bool* using_software_compositing);
  static bool AllowSoftwareToAcceleratedCanvasUpgrade();
  static bool IsValidWithoutRestoring();
  typedef std::function<std::unique_ptr<WebGraphicsContext3DProvider>()>
      ContextProviderFactory;
  static void SetContextProviderFactoryForTesting(ContextProviderFactory);

 private:
  static SharedGpuContext* GetInstanceForCurrentThread();

  SharedGpuContext();
  void CreateContextProviderOnMainThread(WaitableEvent*,
                                         bool* using_software_compositing);
  void CreateContextProviderIfNeeded(bool* using_software_compositing);
  void SetContextProvider(std::unique_ptr<WebGraphicsContext3DProvider>);

  ContextProviderFactory context_provider_factory_ = nullptr;
  std::unique_ptr<WebGraphicsContext3DProviderWrapper>
      context_provider_wrapper_;
  friend class WTF::ThreadSpecific<SharedGpuContext>;
  bool context_provider_creation_failed_ = false;
};

}  // blink

#endif  // SharedGpuContext_h
