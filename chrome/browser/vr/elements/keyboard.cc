// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/keyboard.h"

#include "chrome/browser/vr/controller_mesh.h"
#include "chrome/browser/vr/model/controller_model.h"
#include "chrome/browser/vr/ui_element_renderer.h"

namespace vr {

Keyboard::Keyboard() {
  set_name(kKeyboard);
  SetVisible(false);
}

Keyboard::~Keyboard() = default;

void Keyboard::SetKeyboardDelegate(KeyboardDelegate* keyboard_delegate) {
  delegate_ = keyboard_delegate;
}

void Keyboard::HitTest(const HitTestRequest& request,
                       HitTestResult* result) const {
  result->type = HitTestResult::Type::kNone;
  gfx::Point3F hit_point;
  if (!delegate_->HitTest(request.ray_origin, request.ray_target, &hit_point))
    return;

  float distance_to_plane = (hit_point - request.ray_origin).Length();
  if (distance_to_plane < 0 ||
      distance_to_plane > request.max_distance_to_plane) {
    return;
  }

  result->type = HitTestResult::Type::kHits;
  result->distance_to_plane = distance_to_plane;
  result->hit_point = hit_point;
  result->local_hit_point = gfx::PointF(0, 0);
}

void Keyboard::NotifyClientFloatAnimated(float value,
                                         int target_property_id,
                                         cc::Animation* animation) {
  DCHECK(target_property_id == OPACITY);
  UiElement::NotifyClientFloatAnimated(value, target_property_id, animation);
  if (!delegate_) {
    return;
  }

  if (value == opacity_when_visible()) {
    delegate_->ShowKeyboard();
  } else {
    delegate_->HideKeyboard();
  }
}

void Keyboard::NotifyClientTransformOperationsAnimated(
    const cc::TransformOperations& operations,
    int target_property_id,
    cc::Animation* animation) {
  UiElement::NotifyClientTransformOperationsAnimated(
      operations, target_property_id, animation);
  if (!delegate_)
    return;

  delegate_->SetTransform(LocalTransform());
}

bool Keyboard::OnBeginFrame(const base::TimeTicks& time,
                            const gfx::Vector3dF& head_direction) {
  if (!delegate_)
    return false;

  delegate_->OnBeginFrame();
  return false;
}

void Keyboard::Render(UiElementRenderer* renderer,
                      const CameraModel& camera_model) const {
  if (!delegate_)
    return;

  delegate_->Draw(camera_model);
}

}  // namespace vr
