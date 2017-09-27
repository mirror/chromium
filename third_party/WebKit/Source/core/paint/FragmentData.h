// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FragmentData_h
#define FragmentData_h

#include "core/paint/ClipRects.h"
#include "core/paint/ObjectPaintProperties.h"
#include "core/paint/RarePaintData.h"

namespace blink {

class ObjectPaintProperties;

// Represents the data for a particular fragment of a LayoutObject.
// Only LayoutObjects with a self-painting PaintLayer may have more than one
// FragmentData, and even then only when they are inside of multicol.
// See README.md.
class CORE_EXPORT FragmentData {
 public:
  static std::unique_ptr<FragmentData> Create() {
    return WTF::WrapUnique(new FragmentData());
  }

  FragmentData* NextFragment() const { return next_fragment_.get(); }
  FragmentData& EnsureNextFragment();
  void ClearNextFragment() { next_fragment_.reset(); }

  // The visual paint offset of this fragment. Similar to
  // LayoutObject::PaintOfset, except that the one here takes into account
  // fragmentation.
  LayoutPoint PaintOffset() const { return paint_offset_; }
  void SetPaintOffset(const LayoutPoint& paint_offset) {
    paint_offset_ = paint_offset;
  }

  LayoutRect VisualRect() const { return visual_rect_; }
  void SetVisualRect(const LayoutRect& rect) { visual_rect_ = rect; }

  RarePaintData& EnsureRarePaintData();
  RarePaintData* GetRarePaintData() const { return rare_paint_data_.get(); }
  void ClearRarePaintData() { rare_paint_data_.reset(); }

 private:
  // This stores the visual rect computed by the latest paint invalidation.
  // This rect does *not* account for composited scrolling. See
  // adjustVisualRectForCompositedScrolling().
  LayoutRect visual_rect_;
  LayoutPoint paint_offset_;

  std::unique_ptr<RarePaintData> rare_paint_data_;

  std::unique_ptr<FragmentData> next_fragment_;
};

}  // namespace blink

#endif  // FragmentData_h
