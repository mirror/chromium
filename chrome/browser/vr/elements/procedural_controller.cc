// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/procedural_controller.h"

#include "base/numerics/math_constants.h"
#include "base/trace_event/trace_event.h"
#include "chrome/browser/vr/controller_mesh.h"
#include "chrome/browser/vr/ui_element_renderer.h"
#include "chrome/browser/vr/vr_gl_util.h"

// [0124/183247.441451:INFO:procedural_controller.cc(128)] Min: -0.0175,
// -0.01625, -0.0525
// [0124/183247.441665:INFO:procedural_controller.cc(129)]
// Max: 0.0175, -nan, 0.0525

namespace vr {

namespace {

// clang-format off
static constexpr char const* kVertexShader = SHADER(
  precision mediump float;
  uniform mat4 u_ModelViewProjMatrix;
  attribute vec4 a_Position;
  varying vec4 v_Color;

  void main() {
    v_Color = vec4((a_Position.x + 0.0175) / 0.035,
                    (a_Position.y + 0.01625) / 0.01625,
                    (a_Position.z + 0.0525) / 0.105,
                    1.0);
    gl_Position = u_ModelViewProjMatrix * a_Position;
  }
);

static constexpr char const* kFragmentShader = SHADER(
    precision mediump float;
  uniform float u_Opacity;
  varying vec4 v_Color;

  void main() {
    gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);//v_Color * u_Opacity;
  }
);
// clang-format on

void AddSphere(size_t num_rings,
               size_t num_sectors,
               float arc_rings,
               float arc_sectors,
               const gfx::Transform& transform,
               std::vector<float>* vertices,
               std::vector<GLushort>* indices) {
  const size_t index_offset = vertices->size() / 3;
  const float R = arc_rings / num_rings;
  const float S = arc_sectors / num_sectors;
  const float radius = 0.05;

  vertices->reserve(vertices->size() + (num_rings + 1) * (num_sectors + 1) * 3);
  for (size_t r = 0; r < num_rings + 1; r++) {
    for (size_t s = 0; s < num_sectors + 1; s++) {
      float x = std::sin(2.0 * base::kPiFloat * s * S) *
                std::sin(base::kPiFloat * r * R) * radius;
      float y = std::cos(base::kPiFloat * r * R) * radius;
      float z = std::cos(2.0 * base::kPiFloat * s * S) *
                std::sin(base::kPiFloat * r * R) * radius;

      gfx::Point3F vert(x, y, z);
      //transform.TransformPoint(&vert);
      LOG(INFO) << vert.x() << ", " << vert.y() << ", " << vert.z();
      vertices->push_back(vert.x());
      vertices->push_back(vert.y());
      vertices->push_back(vert.z());
    }
  }

  indices->reserve(indices->size() + num_rings * num_sectors * 6);
  for (size_t r = 0; r < num_rings; r++) {
    size_t r_offset = r * (num_sectors + 1);
    for (size_t s = 0; s < num_sectors; s++) {
      size_t offset = r_offset + s + index_offset;
      // First.
      indices->push_back(offset);                    // -> 0
      indices->push_back(offset + num_sectors + 2);  // -> 6
      indices->push_back(offset + num_sectors + 1);  // -> 5

      // Second.
      indices->push_back(offset);                    // -> 0
      indices->push_back(offset + 1);                // -> 1
      indices->push_back(offset + num_sectors + 2);  // -> 6

      LOG(INFO) << offset;
    }
  }
}

}  // namespace

ProceduralController::ProceduralController() {
  SetName(kController);
  set_hit_testable(false);
  SetVisible(true);
}

ProceduralController::~ProceduralController() = default;

void ProceduralController::Initialize(SkiaSurfaceProvider* provider) {
  AddSphere(20, 20, 1.0f, 1.0f, gfx::Transform(), &vertices_, &indices_);

  glGenBuffersARB(1, &vertex_buffer_);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
  glBufferData(GL_ARRAY_BUFFER, vertices_.size(), vertices_.data(),
               GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glGenBuffersARB(1, &index_buffer_);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_.size(), indices_.data(),
               GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  LOG(INFO) << "initialized";
}

void ProceduralController::Render(UiElementRenderer* renderer,
                                  const CameraModel& model) const {
  ControllerMesh::State state;
  if (touchpad_button_pressed_) {
    state = ControllerMesh::TOUCHPAD;
  } else if (app_button_pressed_) {
    state = ControllerMesh::APP;
  } else if (home_button_pressed_) {
    state = ControllerMesh::SYSTEM;
  } else {
    state = ControllerMesh::IDLE;
  }

  renderer->DrawProceduralController(
      state, computed_opacity(), vertex_buffer_, index_buffer_, indices_.size(),
      model.view_proj_matrix * world_space_transform());
}

gfx::Transform ProceduralController::LocalTransform() const {
  return local_transform_;
}

ProceduralController::Renderer::Renderer()
    : BaseRenderer(kVertexShader, kFragmentShader) {
  model_view_proj_matrix_handle_ =
      glGetUniformLocation(program_handle_, "u_ModelViewProjMatrix");
  opacity_handle_ = glGetUniformLocation(program_handle_, "u_Opacity");
}

ProceduralController::Renderer::~Renderer() = default;

void ProceduralController::Renderer::SetUp(
    std::unique_ptr<ControllerMesh> model) {
  TRACE_EVENT0("gpu", "ControllerRenderer::SetUp");
  // glGenBuffersARB(1, &indices_buffer_);
  // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_buffer_);
  // glBufferData(GL_ELEMENT_ARRAY_BUFFER, model->IndicesBufferSize(),
  //              model->IndicesBuffer(), GL_STATIC_DRAW);

  // glGenBuffersARB(1, &vertex_buffer_);
  // glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
  // glBufferData(GL_ARRAY_BUFFER, model->ElementsBufferSize(),
  //              model->ElementsBuffer(), GL_STATIC_DRAW);

  // const vr::gltf::Accessor* accessor = model->PositionAccessor();
  // position_components_ = vr::gltf::GetTypeComponents(accessor->type);
  // position_type_ = accessor->component_type;
  // position_stride_ = accessor->byte_stride;
  // position_offset_ = VOID_OFFSET(accessor->byte_offset);
  // // LOG(INFO) << "position_components_ " << position_components_;
  // // LOG(INFO) << "position_type_ " << position_type_;
  // // LOG(INFO) << "position_stride_ " << position_stride_;
  // // LOG(INFO) << "position_offset_ " << position_offset_;

  // draw_mode_ = model->DrawMode();
  // accessor = model->IndicesAccessor();
  // indices_count_ = accessor->count;
  // indices_type_ = accessor->component_type;
  // indices_offset_ = VOID_OFFSET(accessor->byte_offset);
  // setup_ = true;

  // float min_x, min_y, min_z = std::numeric_limits<float>::max();
  // float max_x, max_y, max_z = std::numeric_limits<float>::min();
  // for (int i = 0; i < indices_count_; i++) {
  //   auto index = *reinterpret_cast<const int16_t*>(&static_cast<const
  //   char*>(model->IndicesBuffer())[i * 2]); auto x = *reinterpret_cast<const
  //   float*>(&static_cast<const char*>(model->ElementsBuffer())[index * 4 *
  //   3]); auto y = *reinterpret_cast<const float*>(&static_cast<const
  //   char*>(model->ElementsBuffer())[index * 4 * 3 + 4]); auto z =
  //   *reinterpret_cast<const float*>(&static_cast<const
  //   char*>(model->ElementsBuffer())[index * 4 * 3 + 8]); min_x =
  //   std::min(min_x, x); max_x = std::max(max_x, x); min_y = std::min(min_y,
  //   y); max_y = std::max(max_y, y); min_z = std::min(min_z, z); max_z =
  //   std::max(max_z, z);
  // }
  // LOG(INFO) << "Min: " << min_x << ", " << min_y << ", " << min_z;
  // LOG(INFO) << "Max: " << max_x << ", " << max_y << ", " << max_z;

  // // LOG(INFO) << "indices_count_ " << indices_count_;
  // // LOG(INFO) << "draw_mode_ " << draw_mode_;
  // // LOG(INFO) << "indices_type_ " << indices_type_;
  // // LOG(INFO) << "indices_offset_ " << indices_offset_;
}

void ProceduralController::Renderer::Draw(
    ControllerMesh::State state,
    GLuint vertex_buffer,
    GLuint index_buffer,
    size_t index_buffer_size,
    float opacity,
    const gfx::Transform& model_view_proj_matrix) {
  glUseProgram(program_handle_);

  glUniform1f(opacity_handle_, opacity);

  glUniformMatrix4fv(model_view_proj_matrix_handle_, 1, false,
                     MatrixToGLArray(model_view_proj_matrix).data());

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

  glVertexAttribPointer(position_handle_,
    3, GL_FLOAT, false, 3 * sizeof(float), VOID_OFFSET(0));
  // glVertexAttribPointer(position_handle_, position_components_,
  // position_type_,
  //                       GL_FALSE, position_stride_, position_offset_);
  glEnableVertexAttribArray(position_handle_);

  glDisable(GL_BLEND);
  // glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);

  // glClear(GL_COLOR_BUFFER_BIT);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);

  GLenum error;
  while ((error = glGetError()) != (GLenum)GL_NO_ERROR) {
    LOG(INFO) << "error before: " << error;
  }

  // LOG(INFO) << "Render";
  glDrawElements(GL_TRIANGLES, index_buffer_size, GL_UNSIGNED_SHORT, 0);
  // glDrawElements(draw_mode_, indices_count_, indices_type_, indices_offset_);
  error = glGetError();
  if (error != (GLenum)GL_NO_ERROR) {
    LOG(INFO) << "error: " << error;
  }
}

}  // namespace vr
