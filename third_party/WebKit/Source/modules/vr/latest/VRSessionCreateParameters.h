// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VRSessionCreateParameters_h
#define VRSessionCreateParameters_h

#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Forward.h"

namespace blink {

class VRSessionCreateParameters final
    : public GarbageCollected<VRSessionCreateParameters>,
      public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  VRSessionCreateParameters(bool exclusive) : exclusive_(exclusive) {}

  bool exclusive() const { return exclusive_; }

  DEFINE_INLINE_TRACE() {}

 private:
  bool exclusive_;
};

}  // namespace blink

#endif  // VRSessionCreateParameters_h
