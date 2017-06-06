// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_COMPOSITOR_PAINT_CONTEXT_H_
#define UI_COMPOSITOR_PAINT_CONTEXT_H_

#include <memory>

#include "base/logging.h"
#include "base/macros.h"
#include "cc/paint/display_item_list.h"
#include "cc/paint/paint_recorder.h"
#include "ui/compositor/compositor_export.h"
#include "ui/gfx/geometry/rect.h"

namespace cc {
class DisplayItemList;
}

namespace ui {
class ClipRecorder;
class CompositingRecorder;
class PaintRecorder;
class TransformRecorder;

class COMPOSITOR_EXPORT PaintContext {
 public:
  enum ScaleType {
    // Scales the context by the default dsf.
    SCALE_TO_SCALE_FACTOR = 0,

    // Scales the context based on adjusted dsf that is computed for pixel
    // canvas.
    SCALE_TO_FIT
  };

  // Construct a PaintContext that may only re-paint the area in the
  // |invalidation|.
  PaintContext(cc::DisplayItemList* list,
               float device_scale_factor,
               const gfx::Rect& invalidation,
               const gfx::Size& size);

  // Clone a PaintContext with different |bounds| that has some offset from its
  // parent. |scale_type| should be from |ScaleType|.
  PaintContext(const PaintContext& other,
               const gfx::Rect& bounds,
               int scale_type);

  // Clone a PaintContext that has no consideration for invalidation.
  enum CloneWithoutInvalidation {
    CLONE_WITHOUT_INVALIDATION,
  };

  PaintContext(const PaintContext& other, CloneWithoutInvalidation c);

  ~PaintContext();

  // When true, ShouldRepaint() can be called, otherwise its result would be
  // invalid.
  bool CanCheckRepaint() const { return !invalidation_.IsEmpty(); }

  // When true, the |pixel_bounds_| touches an invalidated area, so should be
  // re-painted. When false, re-painting can be skipped.
  bool ShouldRepaint() const {
    DCHECK(CanCheckRepaint());
    return invalidation_.Intersects(gfx::Rect(pixel_size()) + offset_);
  }

#if DCHECK_IS_ON()
  void Visited(void* visited) const {
    if (!root_visited_)
      root_visited_ = visited;
  }
  void* RootVisited() const { return root_visited_; }
  const gfx::Vector2d& PaintOffset() const { return offset_; }
#endif

  const gfx::Rect& InvalidationForTesting() const { return invalidation_; }

  bool IsPixelCanvas() const { return list_ && list_->pixel_canvas_enabled(); }

  // If the |pixel_bounds_| for this context is snapped to its parent's pixel
  // bounds, the effective device scale factor is no longer equal to
  // |device_scale_factor_|. To convert any DIP measurement to its corresponding
  // measurement on the canvas, the following scales must be used.
  float effective_scale_factor_x() const;
  float effective_scale_factor_y() const;

  const gfx::Size& pixel_size() const { return pixel_bounds_.size(); }
  const gfx::Rect& pixel_bounds() const { return pixel_bounds_; }

  // Scales the given DIP measurements to their corresponding canvas measurement
  // using the effective scale factors which may be different from
  // |device_scale_factor_|.
  // Returns the measurements as is, if pixel canvas is disabled.
  gfx::Size ScaleToEffectivePixelSize(const gfx::Size& size) const;
  gfx::Rect ScaleToEffectivePixelBounds(const gfx::Rect& bounds) const;

 private:
  // The Recorder classes need access to the internal canvas and friends, but we
  // don't want to expose them on this class so that people must go through the
  // recorders to access them.
  friend class ClipRecorder;
  friend class CompositingRecorder;
  friend class PaintRecorder;
  friend class TransformRecorder;
  // The Cache class also needs to access the DisplayItemList to append its
  // cache contents.
  friend class PaintCache;

  // Returns a rect with the given size in the space of the context's
  // containing layer.
  gfx::Rect ToLayerSpaceBounds(const gfx::Size& size_in_context) const;

  // Returns the given rect translated by the layer space offset.
  gfx::Rect ToLayerSpaceRect(const gfx::Rect& rect) const;

  // Scales the |child_bounds| to its pixel bounds based on the
  // |device_scale_factor_|. The pixel bounds are snapped to the parent bound's
  // right and/or bottom edge if required.
  // This function returns |child_bounds| as is if pixel canvas is disabled.
  gfx::Rect GetSnappedPixelBounds(const gfx::Rect& child_bounds,
                                  int scale_type) const;

  cc::DisplayItemList* list_;
  std::unique_ptr<cc::PaintRecorder> owned_recorder_;
  // A pointer to the |owned_recorder_| in this PaintContext, or in another one
  // which this was copied from. We expect a copied-from PaintContext to outlive
  // copies made from it.
  cc::PaintRecorder* recorder_;
  // The device scale of the frame being painted. Used to determine which bitmap
  // resources to use in the frame.
  float device_scale_factor_;
  // Invalidation in the space of the paint root (ie the space of the layer
  // backing the paint taking place). This is in pixel size if pixel canvas is
  // enabled.
  gfx::Rect invalidation_;
  // Bounds of the paint context.
  gfx::Rect bounds_;
  // Bounds of paint context in exact pixel size. This will have the same value
  // as |bounds_| if pixel canvas is disabled.
  gfx::Rect pixel_bounds_;
  // Offset from the PaintContext to the space of the paint root and the
  // |invalidation_|. This is in pixels if pixel canvas is enabled.
  gfx::Vector2d offset_;

  int scale_type_ = SCALE_TO_SCALE_FACTOR;

#if DCHECK_IS_ON()
  // Used to verify that the |invalidation_| is only used to compare against
  // rects in the same space.
  mutable void* root_visited_;
  // Used to verify that paint recorders are not nested. True while a paint
  // recorder is active.
  mutable bool inside_paint_recorder_;
#endif

  DISALLOW_COPY_AND_ASSIGN(PaintContext);
};

}  // namespace ui

#endif  // UI_COMPOSITOR_PAINT_CONTEXT_H_
