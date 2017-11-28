// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ClipPathClipper_h
#define ClipPathClipper_h

#include "core/paint/FloatClipRecorder.h"
#include "platform/geometry/FloatRect.h"
#include "platform/graphics/paint/ClipPathRecorder.h"
#include "platform/graphics/paint/CompositingRecorder.h"
#include "platform/wtf/Optional.h"

namespace blink {

class GraphicsContext;
class LayoutObject;

class ClipPathClipper {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

 public:
  ClipPathClipper(GraphicsContext&,
                  const LayoutObject&,
                  const LayoutPoint& paint_offset);
  ~ClipPathClipper();

  bool IsIsolationInstalled() const { return !!mask_isolation_recorder_; }

  static FloatRect LocalReferenceBox(const LayoutObject&);
  static Optional<FloatRect> LocalClipPathBoundingBox(const LayoutObject&);
  static Optional<Path> CanUsePathBasedClip(const LayoutObject&);

 private:
  GraphicsContext& context_;
  const LayoutObject& layout_object_;
  LayoutPoint paint_offset_;

  Optional<FloatClipRecorder> clip_recorder_;
  Optional<ClipPathRecorder> clip_path_recorder_;
  Optional<CompositingRecorder> mask_isolation_recorder_;
};

}  // namespace blink

#endif  // ClipPathClipper_h
