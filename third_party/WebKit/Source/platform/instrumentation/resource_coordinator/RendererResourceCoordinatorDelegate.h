// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RendererResourceCoordinatorDelegate_h
#define RendererResourceCoordinatorDelegate_h

#include <stdint.h>

#include "platform/PlatformExport.h"
#include "platform/wtf/Noncopyable.h"

namespace blink {

class PLATFORM_EXPORT RendererResourceCoordinatorDelegate {
  WTF_MAKE_NONCOPYABLE(RendererResourceCoordinatorDelegate);

 public:
  RendererResourceCoordinatorDelegate();
  static bool IsEnabled();
  static void SetExpectedTaskQueueingDuration(int64_t eqt);
};

}  // namespace blink

#endif  // RendererResourceCoordinatorDelegate_h
