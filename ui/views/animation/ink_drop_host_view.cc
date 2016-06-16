// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/animation/ink_drop_host_view.h"

#include "ui/events/event.h"
#include "ui/events/scoped_target_handler.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/geometry/size_conversions.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/animation/ink_drop_highlight.h"
#include "ui/views/animation/ink_drop_impl.h"
#include "ui/views/animation/ink_drop_stub.h"
#include "ui/views/animation/square_ink_drop_ripple.h"

namespace views {

// Default sizes for ink drop effects.
const int kInkDropSize = 24;
const int kInkDropLargeCornerRadius = 4;

// The scale factor to compute the large ink drop size.
const float kLargeInkDropScale = 1.333f;

namespace {

gfx::Size CalculateLargeInkDropSize(const gfx::Size small_size) {
  return gfx::ScaleToCeiledSize(gfx::Size(small_size), kLargeInkDropScale);
}

}  // namespace

// static
const int InkDropHostView::kInkDropSmallCornerRadius = 2;

// An EventHandler that is guaranteed to be invoked and is not prone to
// InkDropHostView descendents who do not call
// InkDropHostView::OnGestureEvent().
//
// TODO(bruthig): Consider getting rid of this class.
class InkDropHostView::InkDropGestureHandler : public ui::EventHandler {
 public:
  InkDropGestureHandler(View* view, InkDrop* ink_drop)
      : ink_drop_(ink_drop), target_handler_(view, this) {}

  ~InkDropGestureHandler() override {}

  // ui::EventHandler:
  void OnGestureEvent(ui::GestureEvent* event) override {
    InkDropState current_ink_drop_state = ink_drop_->GetTargetInkDropState();

    InkDropState ink_drop_state = InkDropState::HIDDEN;
    switch (event->type()) {
      case ui::ET_GESTURE_TAP_DOWN:
        ink_drop_state = InkDropState::ACTION_PENDING;
        // The ui::ET_GESTURE_TAP_DOWN event needs to be marked as handled so
        // that
        // subsequent events for the gesture are sent to |this|.
        event->SetHandled();
        break;
      case ui::ET_GESTURE_LONG_PRESS:
        ink_drop_state = InkDropState::ALTERNATE_ACTION_PENDING;
        break;
      case ui::ET_GESTURE_LONG_TAP:
        ink_drop_state = InkDropState::ALTERNATE_ACTION_TRIGGERED;
        break;
      case ui::ET_GESTURE_END:
        if (current_ink_drop_state == InkDropState::ACTIVATED)
          return;
      // Fall through to ui::ET_GESTURE_SCROLL_BEGIN case.
      case ui::ET_GESTURE_SCROLL_BEGIN:
        ink_drop_state = InkDropState::HIDDEN;
        break;
      default:
        return;
    }

    if (ink_drop_state == InkDropState::HIDDEN &&
        (current_ink_drop_state == InkDropState::ACTION_TRIGGERED ||
         current_ink_drop_state == InkDropState::ALTERNATE_ACTION_TRIGGERED ||
         current_ink_drop_state == InkDropState::DEACTIVATED)) {
      // These InkDropStates automatically transition to the HIDDEN state so we
      // don't make an explicit call. Explicitly animating to HIDDEN in this
      // case would prematurely pre-empt these animations.
      return;
    }
    ink_drop_->AnimateToState(ink_drop_state);
  }

 private:
  // Animation controller for the ink drop ripple effect.
  InkDrop* ink_drop_;

  // An instance of ScopedTargetHandler allowing |this| to handle events.
  ui::ScopedTargetHandler target_handler_;

  DISALLOW_COPY_AND_ASSIGN(InkDropGestureHandler);
};

InkDropHostView::InkDropHostView()
    : ink_drop_(new InkDropStub()),
      ink_drop_size_(kInkDropSize, kInkDropSize),
      old_paint_to_layer_(false),
      destroying_(false) {}

InkDropHostView::~InkDropHostView() {
  // TODO(bruthig): Improve InkDropImpl to be safer about calling back to
  // potentially destroyed InkDropHosts and remove |destroying_|.
  destroying_ = true;
}

void InkDropHostView::AddInkDropLayer(ui::Layer* ink_drop_layer) {
  old_paint_to_layer_ = layer() != nullptr;
  SetPaintToLayer(true);
  layer()->SetFillsBoundsOpaquely(false);
  layer()->Add(ink_drop_layer);
  layer()->StackAtBottom(ink_drop_layer);
}

void InkDropHostView::RemoveInkDropLayer(ui::Layer* ink_drop_layer) {
  // No need to do anything when called during shutdown, and if a derived
  // class has overridden Add/RemoveInkDropLayer, running this implementation
  // would be wrong.
  if (destroying_)
    return;
  layer()->Remove(ink_drop_layer);
  SetPaintToLayer(old_paint_to_layer_);
}

std::unique_ptr<InkDropRipple> InkDropHostView::CreateInkDropRipple() const {
  std::unique_ptr<InkDropRipple> ripple(new SquareInkDropRipple(
      CalculateLargeInkDropSize(ink_drop_size_), kInkDropLargeCornerRadius,
      ink_drop_size_, kInkDropSmallCornerRadius, GetInkDropCenter(),
      GetInkDropBaseColor()));
  return ripple;
}

std::unique_ptr<InkDropHighlight> InkDropHostView::CreateInkDropHighlight()
    const {
  std::unique_ptr<InkDropHighlight> highlight(
      new InkDropHighlight(ink_drop_size_, kInkDropSmallCornerRadius,
                           GetInkDropCenter(), GetInkDropBaseColor()));
  highlight->set_explode_size(CalculateLargeInkDropSize(ink_drop_size_));
  return highlight;
}

void InkDropHostView::VisibilityChanged(View* starting_from, bool is_visible) {
  View::VisibilityChanged(starting_from, is_visible);
  if (GetWidget() && !is_visible) {
    ink_drop()->AnimateToState(InkDropState::HIDDEN);
    ink_drop()->SetHovered(false);
  }
}

void InkDropHostView::OnFocus() {
  views::View::OnFocus();
  if (ShouldShowInkDropForFocus())
    ink_drop()->SetFocused(true);
}

void InkDropHostView::OnBlur() {
  views::View::OnBlur();
  if (ShouldShowInkDropForFocus())
    ink_drop()->SetFocused(false);
}

void InkDropHostView::OnMouseEvent(ui::MouseEvent* event) {
  switch (event->type()) {
    case ui::ET_MOUSE_ENTERED:
      ink_drop_->SetHovered(true);
      break;
    case ui::ET_MOUSE_EXITED:
      ink_drop_->SetHovered(false);
      break;
    default:
      break;
  }
  View::OnMouseEvent(event);
}

gfx::Point InkDropHostView::GetInkDropCenter() const {
  return GetLocalBounds().CenterPoint();
}

SkColor InkDropHostView::GetInkDropBaseColor() const {
  NOTREACHED();
  return gfx::kPlaceholderColor;
}

bool InkDropHostView::ShouldShowInkDropForFocus() const {
  return false;
}

void InkDropHostView::SetHasInkDrop(bool has_an_ink_drop) {
  if (has_an_ink_drop) {
    ink_drop_.reset(new InkDropImpl(this));
    gesture_handler_.reset(new InkDropGestureHandler(this, ink_drop_.get()));
  } else {
    gesture_handler_.reset();
    ink_drop_.reset(new InkDropStub());
  }
}

void InkDropHostView::AnimateInkDrop(InkDropState ink_drop_state) {
  ink_drop_->AnimateToState(ink_drop_state);
}

}  // namespace views
