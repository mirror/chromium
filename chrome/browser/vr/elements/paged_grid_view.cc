// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/paged_grid_view.h"

#include "cc/animation/animation_curve.h"
#include "chrome/browser/vr/target_property.h"
#include "third_party/WebKit/public/platform/WebGestureEvent.h"

namespace vr {

namespace {
static constexpr float kScrollScaleFactor = 1.0f / 400.0f;
static constexpr float kScrollThreshold = 0.2f;
static constexpr int kScrollTransitionMs = 300;
}  // namespace

PagedGridView::PagedGridView(size_t rows, size_t columns, float margin)
    : rows_(rows), columns_(columns), margin_(margin) {
  DCHECK_NE(0lu, rows);
  DCHECK_NE(0lu, columns);
  set_scrollable(true);
  animation_player().transition().target_properties = {SCROLL_OFFSET};
  animation_player().transition().duration =
      base::TimeDelta::FromMilliseconds(kScrollTransitionMs);
  animation_player().transition().ease_type =
      cc::CubicBezierTimingFunction::EaseType::EASE_OUT;
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
  for (auto* child : children()) {
    if (!child->visible())
      continue;

    size_t child_page = i / (rows_ * columns_);
    size_t in_page_index = i % (rows_ * columns_);
    size_t column = in_page_index % columns_;
    size_t row = in_page_index / columns_;

    gfx::Vector2dF page_offset(ComputePageOffset(child_page), 0.0f);

    if (child_page == CurrentPage()) {
      child->SetOpacity(current_element_opacity_);
      child->set_selectable(true);
    } else if (child_page + 1 == CurrentPage()) {
      child->SetOpacity(ComputeWingOpacity(column));
      child->set_selectable(false);
    } else if (child_page == CurrentPage() + 1) {
      child->SetOpacity(ComputeWingOpacity(columns_ - column - 1));
      child->set_selectable(false);
    } else {
      child->SetOpacity(0.0f);
      child->set_selectable(false);
    }

    gfx::Vector2dF in_page_offset(column * (element_size_.width() + margin_),
                                  row * (element_size_.height() + margin_));

    gfx::Vector2dF child_offset = initial_offset + page_offset + in_page_offset;
    child->SetLayoutOffset(child_offset.x(), -child_offset.y(), 0.001f);
    i++;
  }

  SetSize(columns_ * (element_size_.width() + margin_) + margin_,
          rows_ * (element_size_.height() + margin_) + margin_);
}

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

bool PagedGridView::HitTest(const gfx::PointF& point) const {
  gfx::PointF local_point = point;
  local_point.Scale(size().width(), size().height());
  local_point.Offset(-ComputePageOffset(current_page_), 0.0f);
  return local_point.x() >= 0.0f && local_point.x() <= size().width() &&
         local_point.y() >= 0.0f && local_point.y() <= size().height();
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
  animation_player().TransitionFloatTo(last_frame_time(), SCROLL_OFFSET,
                                       scroll_offset_, offset);
}

float PagedGridView::ComputePageOffset(size_t page) const {
  return page * columns_ * (element_size_.width() + margin_);
}

float PagedGridView::ComputeWingOpacity(size_t column) const {
  return ((column + 1.0f) / columns_) * non_current_element_opacity_;
}

void PagedGridView::OnFlingStart(
    std::unique_ptr<blink::WebGestureEvent> gesture,
    const gfx::PointF& position) {
  // If we're animating in an opposite direction to the given fling direction,
  // then we should attempt to navigate per the fling direction.
  for (auto& animation : animation_player().animations()) {
    if (animation->target_property_id() == SCROLL_OFFSET) {
      const cc::FloatAnimationCurve* curve =
          animation->curve()->ToFloatAnimationCurve();
      float delta = curve->GetValue(curve->Duration()) - scroll_offset_;
      if (delta > 0 != gesture->data.fling_start.velocity_x > 0) {
        size_t next_page = CurrentPage();
        if (next_page + 1 < NumPages() &&
            gesture->data.fling_start.velocity_x < 0) {
          next_page++;
        } else if (next_page > 0 && gesture->data.fling_start.velocity_x > 0) {
          next_page--;
        }
        SetCurrentPage(next_page);
      }
    }
  }
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
