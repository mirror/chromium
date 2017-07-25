// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RendererResourceCoordinator_h
#define RendererResourceCoordinator_h

#include "platform/instrumentation/resource_coordinator/BlinkResourceCoordinatorBase.h"

namespace blink {

class PLATFORM_EXPORT RendererResourceCoordinator final
    : public BlinkResourceCoordinatorBase {
  WTF_MAKE_NONCOPYABLE(RendererResourceCoordinator);

 public:
  static void Initialize();
  static RendererResourceCoordinator* Get();
  ~RendererResourceCoordinator() override;

 private:
  explicit RendererResourceCoordinator(blink::InterfaceProvider*);
};

}  // namespace blink

#endif  // RendererResourceCoordinator_h
