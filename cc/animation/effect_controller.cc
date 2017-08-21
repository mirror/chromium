// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/animation/effect_controller.h"

#include <inttypes.h>
#include <algorithm>

#include "base/stl_util.h"
#include "base/strings/stringprintf.h"
#include "cc/animation/animation_delegate.h"
#include "cc/animation/animation_events.h"
#include "cc/animation/animation_host.h"
#include "cc/animation/animation_timeline.h"
#include "cc/animation/scroll_offset_animation_curve.h"
#include "cc/animation/transform_operations.h"
#include "cc/trees/property_animation_state.h"

namespace cc {

std::unique_ptr<EffectController> EffectController::Create(int id) {
  return base::WrapUnique(new EffectController(id));
}

EffectController::EffectController(int id) : id_(id) {
  DCHECK(id_);
}

EffectController::~EffectController() {
  // DCHECK(!element_animations_);
}

std::unique_ptr<EffectController> EffectController::CreateImplInstance() const {
  std::unique_ptr<EffectController> controller = EffectController::Create(id());
  return controller;
}

void EffectController::SetElementId(ElementId element_id) {
  element_id_ = element_id;
}

}  // namespace cc
