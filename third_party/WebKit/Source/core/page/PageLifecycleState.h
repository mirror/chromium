// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PageLifecycleState_h
#define PageLifecycleState_h

#include "core/CoreExport.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

enum PageLifecycleState {
  kPageLifecycleStateUnknown,
  kPageLifecycleStateStopped,
};

}  // namespace blink
#endif  // ifndef PageLifecycleState _h
