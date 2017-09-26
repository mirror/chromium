// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ng_paint_fragment_h
#define ng_paint_fragment_h

#include "core/layout/ng/ng_physical_fragment.h"
#include "core/loader/resource/ImageResourceObserver.h"
#include "platform/geometry/LayoutRect.h"
#include "platform/graphics/paint/DisplayItemClient.h"
#include "platform/wtf/Allocator.h"

namespace blink {

class NGPaintFragment : public DisplayItemClient, public ImageResourceObserver {
 public:
  explicit NGPaintFragment(RefPtr<const NGPhysicalFragment>);

  const NGPhysicalFragment& PhysicalFragment() const {
    return *physical_fragment_;
  }

  const Vector<std::unique_ptr<NGPaintFragment>>& Children() const { return children_; }

  bool HasOverflowClip() const { return false; }
  LayoutRect VisualRect() const { return visual_rect_; }
  LayoutRect VisualOverflowRect() const { return VisualRect(); }

  String DebugName() const { return "NGPaintFragment"; }

  Node* GetNode() const { return PhysicalFragment().GetNode(); }
  LayoutObject* GetLayoutObject() const { return PhysicalFragment().GetLayoutObject(); }
  const ComputedStyle& Style() const { return PhysicalFragment().Style(); }
  NGPhysicalOffset Offset() const { return PhysicalFragment().Offset(); }
  NGPhysicalSize Size() const { return PhysicalFragment().Size(); }

 private:
  void SetVisualRect(const LayoutRect& rect) { visual_rect_ = rect; }

  void PopulateDescendants();

  RefPtr<const NGPhysicalFragment> physical_fragment_;
  LayoutRect visual_rect_;

  Vector<std::unique_ptr<NGPaintFragment>> children_;
};

}  // namespace blink

#endif  // ng_paint_fragment_h
