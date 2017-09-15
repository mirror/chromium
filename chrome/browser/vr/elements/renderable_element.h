// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_RENDERABLE_ELEMENT_H_
#define CHROME_BROWSER_VR_ELEMENTS_RENDERABLE_ELEMENT_H_

#include "base/macros.h"
#include "chrome/browser/vr/elements/ui_element.h"

namespace vr {

class UiElementRenderer;

// This is the base class for all elements that render.
class RenderableElement : public UiElement {
 public:
  RenderableElement();
  ~RenderableElement() override;

  // Overrides of this method will render itself via |renderer|.
  virtual void Render(UiElementRenderer* renderer,
                      const gfx::Transform& view_proj_matrix) const;

  RenderableElement* AsRenderableElement() final;

 private:
  DISALLOW_COPY_AND_ASSIGN(RenderableElement);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_RENDERABLE_ELEMENT_H_
