// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/ui_input_manager.h"

#include <algorithm>

#include "base/macros.h"
#include "chrome/browser/vr/elements/ui_element.h"
#include "chrome/browser/vr/ui_scene.h"
// TODO(tiborg): Remove include once we use a generic type to pass scroll/fling
// gestures.
#include "third_party/WebKit/public/platform/WebGestureEvent.h"

namespace vr {

namespace {

static constexpr gfx::PointF kInvalidTargetPoint =
    gfx::PointF(std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max());
static constexpr gfx::Point3F kOrigin = {0.0f, 0.0f, 0.0f};

gfx::Point3F GetRayPoint(const gfx::Point3F& rayOrigin,
                         const gfx::Vector3dF& rayVector,
                         float scale) {
  return rayOrigin + gfx::ScaleVector3d(rayVector, scale);
}

bool IsScrollEvent(const GestureList& list) {
  if (list.empty()) {
    return false;
  }
  // We assume that we only need to consider the first gesture in the list.
  blink::WebInputEvent::Type type = list.front()->GetType();
  if (type == blink::WebInputEvent::kGestureScrollBegin ||
      type == blink::WebInputEvent::kGestureScrollEnd ||
      type == blink::WebInputEvent::kGestureScrollUpdate ||
      type == blink::WebInputEvent::kGestureFlingStart ||
      type == blink::WebInputEvent::kGestureFlingCancel) {
    return true;
  }
  return false;
}

}  // namespace

UiInputManager::UiInputManager(UiScene* scene) : scene_(scene) {}

UiInputManager::~UiInputManager() {}

void UiInputManager::HandleInput(const gfx::Vector3dF& laser_direction,
                                 const gfx::Point3F& laser_origin,
                                 ButtonState button_state,
                                 GestureList* gesture_list,
                                 gfx::Point3F* out_target_point,
                                 UiElement** out_reticle_render_target) {
  gfx::PointF target_local_point(kInvalidTargetPoint);
  gfx::Vector3dF eye_to_target;
  *out_reticle_render_target = nullptr;
  HitTestRecords records;
  HitTest(laser_direction, laser_origin, &eye_to_target, &records);

  UiElement* target_element = nullptr;
  // TODO(vollick): this should be replaced with a formal notion of input
  // capture.
  if (input_locked_element_) {
    gfx::Point3F plane_intersection_point;
    float distance_to_plane;
    if (!GetTargetLocalPoint(eye_to_target, *input_locked_element_,
                             &target_local_point, &plane_intersection_point,
                             &distance_to_plane)) {
      target_local_point = kInvalidTargetPoint;
    }
    target_element = input_locked_element_;
  } else if (!in_scroll_ && !in_click_) {
    // TODO(vollick): support hit test opacity. I.e., we may want to dispatch to
    // one of the elements in the list of records. For the moment, we will
    // assume that dispatch is to the first.
    target_element = records.front().element;
    *out_target_point = records.front().target_point;
    if (IsScrollEvent(*gesture_list)) {
      while (!target_element->scrollable() && target_element->parent()) {
        target_element = target_element->parent();
      }
      if (!target_element->scrollable()) {
        target_element = records.front().element;
      }
    }
    *out_reticle_render_target = target_element;
  }

  SendFlingCancel(gesture_list, target_local_point);
  // For simplicity, don't allow scrolling while clicking until we need to.
  if (!in_click_) {
    SendScrollEnd(gesture_list, target_local_point, button_state);
    if (!SendScrollBegin(target_element, gesture_list, target_local_point)) {
      SendScrollUpdate(gesture_list, target_local_point);
    }
  }

  // If we're still scrolling, don't hover (and we can't be clicking, because
  // click ends scroll).
  if (in_scroll_) {
    return;
  }
  SendHoverLeave(target_element);
  if (!SendHoverEnter(target_element, target_local_point)) {
    SendHoverMove(target_local_point);
  }
  SendButtonDown(target_element, target_local_point, button_state);
  SendButtonUp(target_element, target_local_point, button_state);

  previous_button_state_ =
      (button_state == ButtonState::CLICKED) ? ButtonState::UP : button_state;
}

void UiInputManager::SendFlingCancel(GestureList* gesture_list,
                                     const gfx::PointF& target_point) {
  if (!fling_target_) {
    return;
  }
  if (gesture_list->empty() || (gesture_list->front()->GetType() !=
                                blink::WebInputEvent::kGestureFlingCancel)) {
    return;
  }

  // Scrolling currently only supported on content window.
  DCHECK(fling_target_->scrollable());
  fling_target_->OnFlingCancel(std::move(gesture_list->front()), target_point);
  gesture_list->erase(gesture_list->begin());
  fling_target_ = nullptr;
}

void UiInputManager::SendScrollEnd(GestureList* gesture_list,
                                   const gfx::PointF& target_point,
                                   ButtonState button_state) {
  if (!in_scroll_) {
    return;
  }
  DCHECK_NE(input_locked_element_, nullptr);

  if (previous_button_state_ != button_state &&
      (button_state == ButtonState::DOWN ||
       button_state == ButtonState::CLICKED)) {
    DCHECK_GT(gesture_list->size(), 0LU);
    DCHECK_EQ(gesture_list->front()->GetType(),
              blink::WebInputEvent::kGestureScrollEnd);
  }
  DCHECK(input_locked_element_->scrollable());
  if (gesture_list->empty() || (gesture_list->front()->GetType() !=
                                blink::WebInputEvent::kGestureScrollEnd)) {
    return;
  }
  DCHECK_LE(gesture_list->size(), 2LU);
  input_locked_element_->OnScrollEnd(std::move(gesture_list->front()),
                                     target_point);
  gesture_list->erase(gesture_list->begin());
  if (!gesture_list->empty()) {
    DCHECK_EQ(gesture_list->front()->GetType(),
              blink::WebInputEvent::kGestureFlingStart);
    fling_target_ = input_locked_element_;
    fling_target_->OnFlingBegin(std::move(gesture_list->front()), target_point);
    gesture_list->erase(gesture_list->begin());
  }
  input_locked_element_ = nullptr;
  in_scroll_ = false;
}

bool UiInputManager::SendScrollBegin(UiElement* target,
                                     GestureList* gesture_list,
                                     const gfx::PointF& target_point) {
  if (in_scroll_ || !target) {
    return false;
  }
  // Scrolling currently only supported on content window.
  if (!target->scrollable()) {
    return false;
  }
  if (gesture_list->empty() || (gesture_list->front()->GetType() !=
                                blink::WebInputEvent::kGestureScrollBegin)) {
    return false;
  }
  input_locked_element_ = target;
  in_scroll_ = true;

  input_locked_element_->OnScrollBegin(std::move(gesture_list->front()),
                                       target_point);
  gesture_list->erase(gesture_list->begin());
  return true;
}

void UiInputManager::SendScrollUpdate(GestureList* gesture_list,
                                      const gfx::PointF& target_point) {
  if (!in_scroll_) {
    return;
  }
  DCHECK(input_locked_element_);
  if (gesture_list->empty() || (gesture_list->front()->GetType() !=
                                blink::WebInputEvent::kGestureScrollUpdate)) {
    return;
  }
  // Scrolling currently only supported on content window.
  DCHECK(input_locked_element_->scrollable());
  input_locked_element_->OnScrollUpdate(std::move(gesture_list->front()),
                                        target_point);
  gesture_list->erase(gesture_list->begin());
}

void UiInputManager::SendHoverLeave(UiElement* target) {
  if (!hover_target_ || (target == hover_target_)) {
    return;
  }
  hover_target_->OnHoverLeave();
  hover_target_ = nullptr;
}

bool UiInputManager::SendHoverEnter(UiElement* target,
                                    const gfx::PointF& target_point) {
  if (!target || target == hover_target_) {
    return false;
  }
  target->OnHoverEnter(target_point);
  hover_target_ = target;
  return true;
}

void UiInputManager::SendHoverMove(const gfx::PointF& target_point) {
  if (!hover_target_) {
    return;
  }

  // TODO(mthiesse, vollick): Content is currently way too sensitive to mouse
  // moves for how noisy the controller is. It's almost impossible to click a
  // link without unintentionally starting a drag event. For this reason we
  // disable mouse moves, only delivering a down and up event.
  if (hover_target_->fill() == Fill::CONTENT && in_click_) {
    return;
  }

  hover_target_->OnMove(target_point);
}

void UiInputManager::SendButtonDown(UiElement* target,
                                    const gfx::PointF& target_point,
                                    ButtonState button_state) {
  if (in_click_) {
    return;
  }
  if (previous_button_state_ == button_state ||
      (button_state != ButtonState::DOWN &&
       button_state != ButtonState::CLICKED)) {
    return;
  }
  input_locked_element_ = target;
  in_click_ = true;
  if (target) {
    target->OnButtonDown(target_point);
  }
}

void UiInputManager::SendButtonUp(UiElement* target,
                                  const gfx::PointF& target_point,
                                  ButtonState button_state) {
  if (!in_click_) {
    return;
  }
  if (previous_button_state_ == button_state ||
      (button_state != ButtonState::UP &&
       button_state != ButtonState::CLICKED)) {
    return;
  }
  in_click_ = false;
  if (!input_locked_element_) {
    return;
  }
  DCHECK(input_locked_element_ == target);
  input_locked_element_ = nullptr;
  target->OnButtonUp(target_point);
}

void UiInputManager::HitTest(const gfx::Vector3dF& laser_direction,
                             const gfx::Point3F& laser_origin,
                             gfx::Vector3dF* eye_to_target,
                             HitTestRecords* records) const {
  float distance = scene_->background_distance();
  gfx::Point3F target_point =
      GetRayPoint(laser_origin, laser_direction, distance);
  *eye_to_target = target_point - kOrigin;
  eye_to_target->GetNormalized(eye_to_target);

  for (auto& element : scene_->GetUiElements()) {
    if (!element->IsHitTestable()) {
      continue;
    }
    gfx::PointF local_point;
    gfx::Point3F plane_intersection_point;
    float distance_to_plane;
    if (!GetTargetLocalPoint(*eye_to_target, *element, &local_point,
                             &plane_intersection_point, &distance_to_plane)) {
      continue;
    }
    if (!element->HitTest(local_point)) {
      continue;
    }

    HitTestRecord record;
    record.element = element.get();
    record.depth = distance_to_plane;
    record.target_point = plane_intersection_point;
    record.target_local_point = local_point;
    records->push_back(record);
  }

  std::sort(records->begin(), records->end(),
            [](const HitTestRecord& lhs, const HitTestRecord& rhs) -> bool {
              return lhs.depth < rhs.depth;
            });
}

bool UiInputManager::GetTargetLocalPoint(const gfx::Vector3dF& eye_to_target,
                                         const UiElement& element,
                                         gfx::PointF* out_target_local_point,
                                         gfx::Point3F* out_target_point,
                                         float* out_distance_to_plane) const {
  if (!element.GetRayDistance(kOrigin, eye_to_target, out_distance_to_plane)) {
    return false;
  }

  if (*out_distance_to_plane < 0) {
    return false;
  }

  *out_target_point =
      GetRayPoint(kOrigin, eye_to_target, *out_distance_to_plane);
  gfx::PointF unit_xy_point =
      element.GetUnitRectangleCoordinates(*out_target_point);

  out_target_local_point->set_x(0.5f + unit_xy_point.x());
  out_target_local_point->set_y(0.5f - unit_xy_point.y());
  return true;
}

}  // namespace vr
