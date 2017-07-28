// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/paged_grid_view.h"

#include "chrome/browser/vr/target_property.h"
#include "third_party/WebKit/public/platform/WebGestureEvent.h"

namespace vr {

namespace {
static constexpr float kScrollScaleFactor = 1.0f / 400.0f;
static constexpr float kScrollThreshold = 0.2f;
}  // namespace

PagedGridView::PagedGridView(size_t rows, size_t columns, float margin)
    : rows_(rows), columns_(columns), margin_(margin) {
  DCHECK_NE(0lu, rows);
  DCHECK_NE(0lu, columns);
}

PagedGridView::~PagedGridView() {}

void PagedGridView::LayOutChildren() {
  // First, the position of our children with respect to us is as follows. The
  // first paged is centered about our origin. Subsequent pages are positioned
  // to the right.
  //
  // NB: scrolling is NOT handled in this function. Rather it is driven by our
  // inheritable transform and is a function of two values.
  //   1. Current page, and
  //   2. Current gesture / animation.
  element_size_ = gfx::SizeF();
  page_size_ = gfx::SizeF();
  if (!children().empty()) {
    element_size_ = children().front()->size();
    page_size_ =
        gfx::SizeF(columns_ * element_size_.width() + (columns_ - 1) * margin_,
                   rows_ * element_size_.height() + (rows_ - 1) * margin_);
  }

  gfx::Vector2dF initial_offset(
      -0.5 * (page_size_.width() - element_size_.width()),
      -0.5 * (page_size_.height() - element_size_.height()));

  SetCurrentPage(CurrentPage());

  size_t i = 0;
  float total_extent = 0.0f;
  for (auto* child : children()) {
    if (!child->visible())
      continue;

    size_t child_page = i / (rows_ * columns_);
    gfx::Vector2dF page_offset(ComputePageOffset(child_page), 0.0f);

    // TODO(vollick): if we had a view-model, we could set a state here
    // indicating that it's in the wings and the view could do whatever it
    // wanted in response. This hardcoding of opacity is unfortunate, though
    // probably ok for us.
    float opacity = 0.0f;
    if (child_page == CurrentPage()) {
      opacity = current_element_opacity_;
    } else if (child_page + 1 == CurrentPage() ||
               child_page == CurrentPage() + 1) {
      opacity = non_current_element_opacity_;
    }
    child->SetOpacity(opacity);

    size_t in_page_index = i % (rows_ * columns_);
    gfx::Vector2dF in_page_offset(
        (in_page_index % columns_) * (element_size_.width() + margin_),
        (in_page_index / columns_) * (element_size_.height() + margin_));

    gfx::Vector2dF child_offset = initial_offset + page_offset + in_page_offset;
    child->SetLayoutOffset(child_offset.x(), -child_offset.y(), 0.001f);
    total_extent = child_offset.x() + 0.5 * element_size_.width();
    i++;
  }

  SetSize(total_extent * 2.0f, size().height());
}

// TODO(vollick): Handle input.
size_t PagedGridView::NumPages() const {
  size_t page_size = rows_ * columns_;
  size_t num_pages = children().size() / page_size;
  if (children().size() % page_size)
    num_pages++;
  return num_pages;
}

size_t PagedGridView::CurrentPage() const {
  return current_page_;
}

void PagedGridView::SetCurrentPage(size_t current_page) {
  current_page_ = current_page;
  if (NumPages() == 0) {
    current_page_ = 0;
  } else if (current_page >= NumPages()) {
    current_page_ = NumPages() - 1;
  }
  // TODO(vollick): this should be called after layout. Need to add a DCHECK to
  // ensure that we're in the correct phase of the frame lifecycle when this is
  // invoked.
  SetScrollOffset(-ComputePageOffset(current_page_));
}

gfx::Transform PagedGridView::LocalTransform() const {
  gfx::Transform to_return = UiElement::LocalTransform();
  to_return.matrix().postTranslate(scroll_offset_ + scroll_drag_delta_, 0.0f,
                                   0.0f);
  return to_return;
}

void PagedGridView::NotifyClientFloatAnimated(float value,
                                              int target_property_id,
                                              cc::Animation* animation) {
  if (target_property_id == SCROLL_OFFSET) {
    scroll_offset_ = value;
  } else {
    UiElement::NotifyClientFloatAnimated(value, target_property_id, animation);
  }
}

void PagedGridView::SetScrollOffset(float offset) {
  animation_player().TransitionFloatTo(
      last_frame_time(), TargetProperty::SCROLL_OFFSET, scroll_offset_, offset);
}

float PagedGridView::ComputePageOffset(size_t page) {
  return page * columns_ * (element_size_.width() + margin_);
}

void PagedGridView::OnScrollBegin(
    std::unique_ptr<blink::WebGestureEvent> gesture,
    const gfx::PointF& position) {
  cached_transition_ = animation_player().transition();
  animation_player().set_transition(Transition());
  scroll_drag_delta_ = 0.0f;
}
void PagedGridView::OnScrollUpdate(
    std::unique_ptr<blink::WebGestureEvent> gesture,
    const gfx::PointF& position) {
  scroll_drag_delta_ +=
      gesture->data.scroll_update.delta_x * kScrollScaleFactor;
}
void PagedGridView::OnScrollEnd(std::unique_ptr<blink::WebGestureEvent> gesture,
                                const gfx::PointF& position) {
  size_t next_page = CurrentPage();
  if (next_page + 1 < NumPages() && scroll_drag_delta_ < -kScrollThreshold) {
    next_page++;
  } else if (next_page > 0 && scroll_drag_delta_ > kScrollThreshold) {
    next_page--;
  }
  scroll_offset_ += scroll_drag_delta_;
  scroll_drag_delta_ = 0;
  animation_player().set_transition(cached_transition_);
  SetCurrentPage(next_page);
}

}  // namespace vr
