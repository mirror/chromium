// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ng_fragment_painter_h
#define ng_fragment_painter_h

#include "core/paint/ObjectPainterBase.h"
#include "platform/wtf/Allocator.h"

namespace blink {

class LayoutPoint;
class NGPaintFragment;
struct PaintInfo;

// Generic fragment painter for paint logic shared between all types of
// fragments. LayoutNG version of ObjectPainter, based on ObjectPainterBase.
class NGFragmentPainter : public ObjectPainterBase {
  STACK_ALLOCATED();

 public:
  NGFragmentPainter(const NGPaintFragment& paint_fragment)
      : paint_fragment_(paint_fragment) {}

  void PaintOutline(const PaintInfo&, const LayoutPoint& paint_offset);

  // We compute paint offsets during the pre-paint tree walk (PrePaintTreeWalk).
  // This check verifies that the paint offset computed during pre-paint matches
  // the actual paint offset during paint.
  void CheckPaintOffset(const PaintInfo& paint_info,
                        const LayoutPoint& paint_offset) {
#if DCHECK_IS_ON()
    // For now this works for SPv2 only because of complexities of paint
    // invalidation containers in SPv1.
    if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled())
      DoCheckPaintOffset(paint_info, paint_offset);
#endif
  }

 private:
#if DCHECK_IS_ON()
  void DoCheckPaintOffset(const PaintInfo&, const LayoutPoint& paint_offset);
#endif

  const NGPaintFragment& paint_fragment_;
};

}  // namespace blink

#endif
