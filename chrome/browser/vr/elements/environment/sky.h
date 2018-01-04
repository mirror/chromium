// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_ENVIRONMENT_SKY_H_
#define CHROME_BROWSER_VR_ELEMENTS_ENVIRONMENT_SKY_H_

#include <vector>

#include "base/time/time.h"
#include "chrome/browser/vr/elements/ui_element.h"
#include "chrome/browser/vr/model/color_scheme.h"
#include "chrome/browser/vr/renderers/base_renderer.h"

namespace vr {

struct EnvironmentGradientColors;

// Renders a gradient across a sphere. Can be used, for example,  to overlay a
// gradient atop other environmental elements or on its own. We currently render
// the entire environmental gradient, not just the sky.
// TODO(vollick): once we have ground elements, we can elide the bottom half of
// the sphere.
class Sky : public UiElement {
 public:
  Sky();
  ~Sky() override;

  void Render(UiElementRenderer* renderer,
              const CameraModel& camera) const override;

  void SetColors(const EnvironmentGradientColors& colors);

  class Renderer : public BaseRenderer {
   public:
    Renderer();
    ~Renderer() override;
    void Draw(const gfx::Transform& view_proj_matrix,
              const EnvironmentGradientColors& colors,
              float opacity);

    static void CreateBuffers();

   private:
    static GLuint vertex_buffer_;
    static GLuint index_buffer_;

    // Uniforms
    GLuint model_view_proj_matrix_handle_ = 0;
    GLuint ground_color_handle_ = 0;
    GLuint mid_ground_color_handle_ = 0;
    GLuint below_horizon_color_handle_ = 0;
    GLuint horizon_color_handle_ = 0;
    GLuint above_horizon_color_handle_ = 0;
    GLuint mid_sky_color_handle_ = 0;
    GLuint sky_color_handle_ = 0;
    GLuint opacity_handle_ = 0;

    // Attributes
    GLuint ground_weight_handle_ = 0;
    GLuint mid_ground_weight_handle_ = 0;
    GLuint below_horizon_weight_handle_ = 0;
    GLuint horizon_weight_handle_ = 0;
    GLuint above_horizon_weight_handle_ = 0;
    GLuint mid_sky_weight_handle_ = 0;
    GLuint sky_weight_handle_ = 0;

    DISALLOW_COPY_AND_ASSIGN(Renderer);
  };

 private:
  void NotifyClientColorAnimated(SkColor value,
                                 int target_property_id,
                                 cc::Animation* animation) override;
  EnvironmentGradientColors colors_;
  DISALLOW_COPY_AND_ASSIGN(Sky);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_ENVIRONMENT_SKY_H_
