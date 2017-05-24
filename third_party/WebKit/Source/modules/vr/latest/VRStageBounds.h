// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VRStageBounds_h
#define VRStageBounds_h

#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Forward.h"

namespace blink {

class VRStageBounds final : public GarbageCollected<VRStageBounds>,
                            public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  VRStageBounds(double min_x, double max_x, double min_z, double max_z)
      : min_x_(min_x), max_x_(max_x), min_z_(min_z), max_z_(max_z) {}

  double minX() const { return min_x_; }
  double maxX() const { return max_x_; }
  double minZ() const { return min_z_; }
  double maxZ() const { return max_z_; }

  DEFINE_INLINE_TRACE() {}

 private:
  double min_x_;
  double max_x_;
  double min_z_;
  double max_z_;
};

}  // namespace blink

#endif  // VRStageBounds_h
