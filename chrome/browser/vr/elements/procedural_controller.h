// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_PROCEDURAL_CONTROLLER_H_
#define CHROME_BROWSER_VR_ELEMENTS_PROCEDURAL_CONTROLLER_H_

#include <vector>

#include "base/macros.h"
#include "chrome/browser/vr/controller_mesh.h"
#include "chrome/browser/vr/elements/ui_element.h"
#include "chrome/browser/vr/renderers/base_renderer.h"

namespace vr {

// This represents the controller.
class ProceduralController : public UiElement {
 public:
  ProceduralController();
  ~ProceduralController() override;

  void Initialize(SkiaSurfaceProvider* provider) override;

  void set_touchpad_button_pressed(bool pressed) {
    touchpad_button_pressed_ = pressed;
  }

  void set_app_button_pressed(bool pressed) { app_button_pressed_ = pressed; }

  void set_home_button_pressed(bool pressed) { home_button_pressed_ = pressed; }

  void set_local_transform(const gfx::Transform& transform) {
    local_transform_ = transform;
  }

  class Renderer : public BaseRenderer {
   public:
    Renderer();
    ~Renderer() override;

    void SetUp(std::unique_ptr<ControllerMesh> model);
    void Draw(ControllerMesh::State state,
              GLuint vertex_buffer,
              GLuint index_buffer,
              size_t index_buffer_size,
              float opacity,
              const gfx::Transform& view_proj_matrix);
    bool IsSetUp() const { return true; }

   private:
    GLuint model_view_proj_matrix_handle_;
    GLuint opacity_handle_;
    // GLint position_components_ = 0;
    // GLenum position_type_ = GL_FLOAT;
    // GLsizei position_stride_ = 0;
    // const GLvoid* position_offset_ = nullptr;
    // GLenum draw_mode_ = GL_TRIANGLES;
    // GLsizei indices_count_ = 0;
    // GLenum indices_type_ = GL_INT;
    // const GLvoid* indices_offset_ = nullptr;
    // bool setup_ = false;

    DISALLOW_COPY_AND_ASSIGN(Renderer);
  };

 private:
  void Render(UiElementRenderer* renderer,
              const CameraModel& model) const final;

  gfx::Transform LocalTransform() const override;

  std::vector<float> vertices_;
  std::vector<GLushort> indices_;
  GLuint vertex_buffer_ = 0;
  GLuint index_buffer_ = 0;
  bool touchpad_button_pressed_ = false;
  bool app_button_pressed_ = false;
  bool home_button_pressed_ = false;
  gfx::Transform local_transform_;

  DISALLOW_COPY_AND_ASSIGN(ProceduralController);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_PROCEDURAL_CONTROLLER_H_
