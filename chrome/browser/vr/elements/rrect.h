// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_RRECT_H_
#define CHROME_BROWSER_VR_ELEMENTS_RRECT_H_

#include "chrome/browser/vr/elements/ui_element.h"
#include "third_party/skia/include/core/SkColor.h"

namespace vr {

// A rounded rect sizes itself to contain its children (plus padding, if
// applicable) and is able to render.
class RRect : public UiElement {
 public:
  RRect();
  ~RRect() override;

  float padding() const { return padding_; }
  void set_padding(float padding) { padding_ = padding; }

  SkColor center_color() const { return center_color_; }
  void SetCenterColor(SkColor color);

  SkColor edge_color() const { return edge_color_; }
  void SetEdgeColor(SkColor color);

  void NotifyClientColorAnimated(SkColor color,
                                 int target_property_id,
                                 cc::Animation* animation) override;

  void Render(UiElementRenderer* renderer,
              const gfx::Transform& view_proj_matrix) const override;

 private:
  float padding_ = 0.0f;
  SkColor center_color_ = SK_ColorWHITE;
  SkColor edge_color_ = SK_ColorWHITE;

  DISALLOW_COPY_AND_ASSIGN(RRect);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_RRECT_H_
