// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/vr_shell/ui_input_manager.h"

#include <chrono>
#include <limits>
#include <utility>

#include "base/callback_helpers.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/android/vr_shell/fps_meter.h"
#include "chrome/browser/android/vr_shell/gl_browser_interface.h"
#include "chrome/browser/android/vr_shell/mailbox_to_surface_bridge.h"
#include "chrome/browser/android/vr_shell/ui_elements/ui_element.h"
#include "chrome/browser/android/vr_shell/ui_interface.h"
#include "chrome/browser/android/vr_shell/ui_scene.h"
#include "chrome/browser/android/vr_shell/vr_controller.h"
#include "chrome/browser/android/vr_shell/vr_gl_util.h"
#include "chrome/browser/android/vr_shell/vr_shell.h"
#include "chrome/browser/android/vr_shell/vr_shell_renderer.h"
#include "device/vr/android/gvr/gvr_delegate.h"
#include "device/vr/android/gvr/gvr_device.h"
#include "device/vr/android/gvr/gvr_gamepad_data_provider.h"
#include "ui/gl/android/scoped_java_surface.h"
#include "ui/gl/android/surface_texture.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_fence_egl.h"
#include "ui/gl/gl_surface.h"
#include "ui/gl/init/gl_factory.h"

namespace vr_shell {

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

}  // namespace

UiInputManagerDelegate::~UiInputManagerDelegate() = default;

UiInputManager::UiInputManager(UiScene* scene, UiInputManagerDelegate* delegate)
    : scene_(scene), delegate_(delegate) {}

UiInputManager::~UiInputManager() {}

void UiInputManager::UpdateController(
    const gfx::Vector3dF& controller_direction,
    const gfx::Point3F& pointer_start,
    ButtonState button_state) {
  pointer_start_ = pointer_start;
  gfx::PointF target_local_point(kInvalidTargetPoint);
  gfx::Vector3dF eye_to_target;
  reticle_render_target_ = nullptr;
  GetVisualTargetElement(controller_direction, eye_to_target, target_point_,
                         &reticle_render_target_, target_local_point);

  UiElement* target_element = nullptr;
  if (input_locked_element_) {
    gfx::Point3F plane_intersection_point;
    float distance_to_plane;
    if (!GetTargetLocalPoint(eye_to_target, *input_locked_element_,
                             2 * scene_->GetBackgroundDistance(),
                             target_local_point, plane_intersection_point,
                             distance_to_plane)) {
      target_local_point = kInvalidTargetPoint;
    }
    target_element = input_locked_element_;
  } else if (!in_scroll_ && !in_click_) {
    target_element = reticle_render_target_;
  }

  // If we're still scrolling, don't hover (and we can't be clicking, because
  // click ends scroll).
  if (in_scroll_)
    return;
  SendHoverLeave(target_element);
  if (!SendHoverEnter(target_element, target_local_point)) {
    SendHoverMove(target_local_point);
  }
  SendButtonDown(target_element, target_local_point, button_state);
  SendButtonUp(target_element, target_local_point, button_state);
}

gfx::Point3F UiInputManager::GetTargetPoint() {
  return target_point_;
}

UiElement* UiInputManager::GetReticleRenderTarget() {
  return reticle_render_target_;
}

void UiInputManager::SendHoverLeave(UiElement* target) {
  if (!hover_target_ || (target == hover_target_))
    return;
  if (hover_target_->fill() == Fill::CONTENT) {
    delegate_->OnExitWebContents();
  } else {
    hover_target_->OnHoverLeave();
  }
  hover_target_ = nullptr;
}

bool UiInputManager::SendHoverEnter(UiElement* target,
                                    const gfx::PointF& target_point) {
  if (!target || target == hover_target_)
    return false;
  if (target->fill() == Fill::CONTENT) {
    delegate_->OnEnterWebContents(target_point);
  } else {
    target->OnHoverEnter(target_point);
  }
  hover_target_ = target;
  return true;
}

void UiInputManager::SendHoverMove(const gfx::PointF& target_point) {
  if (!hover_target_)
    return;
  if (hover_target_->fill() == Fill::CONTENT) {
    delegate_->OnMoveOnWebContents(target_point);
  } else {
    hover_target_->OnMove(target_point);
  }
}

void UiInputManager::SendButtonDown(UiElement* target,
                                    const gfx::PointF& target_point,
                                    ButtonState button_state) {
  if (in_click_)
    return;
  if (previous_button_state_ == button_state ||
      (button_state != ButtonState::DOWN && button_state != ButtonState::CLICK))
    return;

  input_locked_element_ = target;
  in_click_ = true;
  if (!target)
    return;
  if (target->fill() == Fill::CONTENT) {
    delegate_->OnDownOnWebContents(target_point);
  } else {
    target->OnButtonDown(target_point);
  }
}

void UiInputManager::SendButtonUp(UiElement* target,
                                  const gfx::PointF& target_point,
                                  ButtonState button_state) {
  if (!in_click_)
    return;
  if (previous_button_state_ == button_state ||
      (button_state != ButtonState::UP && button_state != ButtonState::CLICK))
    return;
  in_click_ = false;
  if (!input_locked_element_)
    return;
  DCHECK(input_locked_element_ == target);
  input_locked_element_ = nullptr;
  if (target->fill() == Fill::CONTENT) {
    delegate_->OnUpOnWebContents(target_point);
  } else {
    target->OnButtonUp(target_point);
  }
}

void UiInputManager::GetVisualTargetElement(
    const gfx::Vector3dF& controller_direction,
    gfx::Vector3dF& eye_to_target,
    gfx::Point3F& target_point,
    UiElement** target_element,
    gfx::PointF& target_local_point) const {
  // If we place the reticle based on elements intersecting the controller beam,
  // we can end up with the reticle hiding behind elements, or jumping laterally
  // in the field of view. This is physically correct, but hard to use. For
  // usability, do the following instead:
  //
  // - Project the controller laser onto a distance-limiting sphere.
  // - Create a vector between the eyes and the outer surface point.
  // - If any UI elements intersect this vector, and is within the bounding
  //   sphere, choose the closest to the eyes, and place the reticle at the
  //   intersection point.

  // Compute the distance from the eyes to the distance limiting sphere. Note
  // that the sphere is centered at the controller, rather than the eye, for
  // simplicity.
  float distance = scene_->GetBackgroundDistance();
  target_point = GetRayPoint(pointer_start_, controller_direction, distance);
  eye_to_target = target_point - kOrigin;
  eye_to_target.GetNormalized(&eye_to_target);

  // Determine which UI element (if any) intersects the line between the eyes
  // and the controller target position.
  float closest_element_distance = (target_point - kOrigin).Length();

  for (auto& element : scene_->GetUiElements()) {
    if (!element->IsHitTestable())
      continue;
    gfx::PointF local_point;
    gfx::Point3F plane_intersection_point;
    float distance_to_plane;
    if (!GetTargetLocalPoint(eye_to_target, *element.get(),
                             closest_element_distance, local_point,
                             plane_intersection_point, distance_to_plane))
      continue;
    if (!element->HitTest(local_point))
      continue;

    closest_element_distance = distance_to_plane;
    target_point = plane_intersection_point;
    *target_element = element.get();
    target_local_point = local_point;
  }
}

bool UiInputManager::GetTargetLocalPoint(const gfx::Vector3dF& eye_to_target,
                                         const UiElement& element,
                                         float max_distance_to_plane,
                                         gfx::PointF& target_local_point,
                                         gfx::Point3F& target_point,
                                         float& distance_to_plane) const {
  if (!element.GetRayDistance(kOrigin, eye_to_target, &distance_to_plane))
    return false;

  if (distance_to_plane < 0 || distance_to_plane >= max_distance_to_plane)
    return false;

  target_point = GetRayPoint(kOrigin, eye_to_target, distance_to_plane);
  gfx::PointF unit_xy_point = element.GetUnitRectangleCoordinates(target_point);

  target_local_point.set_x(0.5f + unit_xy_point.x());
  target_local_point.set_y(0.5f - unit_xy_point.y());
  return true;
}

}  // namespace vr_shell
