// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_PAGED_GRID_VIEW_H_
#define CHROME_BROWSER_VR_ELEMENTS_PAGED_GRID_VIEW_H_

#include "base/macros.h"
#include "chrome/browser/vr/elements/ui_element.h"
#include "chrome/browser/vr/transition.h"

namespace vr {

// A paged grid view is comprized of parts, the pages (and adjacent wings).
//
//   a b c G H I m n o
//   d e f J K L p q r
//
// The capital letters are the current page of elements, and the lower case
// elements represent the elements in the wings. The order of the elements
// follows the alphabetical ordering pictured above.
//
// NB: it is assumed that each child view has the same dimensions.
class PagedGridView : public UiElement {
 public:
  PagedGridView(size_t rows, size_t columns, float margin);
  ~PagedGridView() override;

  void LayOutChildren() override;

  // TODO(vollick): Handle input.
  size_t NumPages() const;
  size_t CurrentPage() const;
  void SetCurrentPage(size_t current_page);

 private:
  gfx::Transform LocalTransform() const override;
  void NotifyClientFloatAnimated(float opacity,
                                 int target_property_id,
                                 cc::Animation* animation) override;
  void SetScrollOffset(float offset);
  float ComputePageOffset(size_t page);

  void OnScrollBegin(std::unique_ptr<blink::WebGestureEvent> gesture,
                     const gfx::PointF& position) override;
  void OnScrollUpdate(std::unique_ptr<blink::WebGestureEvent> gesture,
                      const gfx::PointF& position) override;
  void OnScrollEnd(std::unique_ptr<blink::WebGestureEvent> gesture,
                   const gfx::PointF& position) override;

  size_t rows_;
  size_t columns_;
  float margin_;
  size_t current_page_ = 0;

  float non_current_element_opacity_ = 0.1;
  float current_element_opacity_ = 1.0;

  float scroll_offset_ = 0.0f;
  float scroll_drag_delta_ = 0.0f;

  // This is presumed to be uniform across children. This, and page size below,
  // are measured at the time of layout.
  gfx::SizeF element_size_;
  gfx::SizeF page_size_;

  Transition cached_transition_;

  DISALLOW_COPY_AND_ASSIGN(PagedGridView);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_PAGED_GRID_VIEW_H_
