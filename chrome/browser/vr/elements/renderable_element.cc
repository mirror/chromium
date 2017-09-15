// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/renderable_element.h"

namespace vr {
RenderableElement::RenderableElement() = default;
RenderableElement::~RenderableElement() = default;

void RenderableElement::Render(UiElementRenderer* renderer,
                               const gfx::Transform& view_proj_matrix) const {}

RenderableElement* RenderableElement::AsRenderableElement() {
  return this;
}

}  // namespace vr
