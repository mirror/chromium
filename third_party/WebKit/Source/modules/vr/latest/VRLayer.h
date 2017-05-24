// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VRLayer_h
#define VRLayer_h

#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Forward.h"

namespace blink {

class VRLayer : public GarbageCollected<VRLayer>, public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  VRLayer();

  DECLARE_VIRTUAL_TRACE();
};

}  // namespace blink

#endif  // VRLayer_h
