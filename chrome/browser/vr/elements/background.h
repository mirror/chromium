// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_BACKGROUND_H_
#define CHROME_BROWSER_VR_ELEMENTS_BACKGROUND_H_

#include "chrome/browser/vr/elements/ui_element.h"
#include "chrome/browser/vr/renderers/base_quad_renderer.h"
#include "third_party/skia/include/core/SkRefCnt.h"
#include "ui/gl/gl_bindings.h"

class SkSurface;

namespace vr {

class Background : public UiElement {
 public:
  Background();
  ~Background() override;

  void Render(UiElementRenderer* renderer,
              const CameraModel& model) const override;
  void Initialize(SkiaSurfaceProvider* provider) override;

  class Renderer : public BaseRenderer {
   public:
    Renderer();
    ~Renderer() final;

    void Draw(const gfx::Transform& view_proj_matrix,
              int texture_data_handle,
              float opacity);

   private:
    GLuint model_view_proj_matrix_handle_;
    GLuint opacity_handle_;
    GLuint tex_uniform_handle_;

    GLuint vertex_buffer_;
    GLuint index_buffer_;
    GLuint index_count_;

    DISALLOW_COPY_AND_ASSIGN(Renderer);
  };

 private:
  GLuint texture_handle_;
  sk_sp<SkSurface> surface_;

  DISALLOW_COPY_AND_ASSIGN(Background);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_BACKGROUND_H_
