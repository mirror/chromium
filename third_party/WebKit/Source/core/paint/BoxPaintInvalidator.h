// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BoxPaintInvalidator_h
#define BoxPaintInvalidator_h

#include "core/CoreExport.h"
#include "platform/graphics/PaintInvalidationReason.h"
#include "platform/wtf/Allocator.h"

namespace blink {

class LayoutBox;
class LayoutRect;
struct PaintInvalidatorContext;

class CORE_EXPORT BoxPaintInvalidator {
  STACK_ALLOCATED();

 public:
  BoxPaintInvalidator(const LayoutBox& box,
                      const PaintInvalidatorContext& context)
      : box_(box), context_(context) {}

  static void BoxWillBeDestroyed(const LayoutBox&);

  PaintInvalidationReason InvalidatePaint();

 private:
  friend class BoxPaintInvalidatorTest;

  bool BackgroundGeometryDependsOnLayoutOverflowRect();
  bool BackgroundPaintsOntoScrollingContentsLayer();
  bool ShouldFullyInvalidateBackgroundOnLayoutOverflowChange(
      const LayoutRect& old_layout_overflow,
      const LayoutRect& new_layout_overflow);

  enum BackgroundInvalidationType {
    kNone = 0,
    kIncremental,
    kFullWithoutGeometryChange,
    kFullWithGeometryChange,
  };
  BackgroundInvalidationType ComputeBackgroundInvalidation();
  void InvalidateScrollingContentsBackground(BackgroundInvalidationType);

  PaintInvalidationReason ComputePaintInvalidationReason();

  // The box for background size computations. Typically |box_| but can be
  // different for LayoutView which can use the <html>'s size.
  const LayoutBox& BackgroundBox() const;
  bool BackgroundBoxSizeChanged() const;

  void IncrementallyInvalidatePaint(PaintInvalidationReason,
                                    const LayoutRect& old_rect,
                                    const LayoutRect& new_rect);

  bool NeedsToSavePreviousContentBoxSizeOrLayoutOverflowRect();
  void SavePreviousBoxGeometriesIfNeeded();

  const LayoutBox& box_;
  const PaintInvalidatorContext& context_;
};

}  // namespace blink

#endif
