// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/environment/sky.h"

#include "base/numerics/math_constants.h"
#include "base/rand_util.h"
#include "cc/animation/keyframed_animation_curve.h"
#include "chrome/browser/vr/ui_element_renderer.h"
#include "chrome/browser/vr/vr_gl_util.h"
#include "ui/gfx/animation/tween.h"

namespace vr {

namespace {

// clang-format off
constexpr size_t kLinesLongitude = 10lu;
constexpr size_t kLinesLatitude = 36lu;
constexpr float kColorStopTimeFractions[] = {
    0.0f,  // ground
    0.43f, // mid ground
    0.485f, // below horizon
    0.495f,  // horizon
    0.505f, // above horizon
    0.6f,  // mid sky
    1.0f   // sky
};
constexpr size_t kNumColorStops = arraysize(kColorStopTimeFractions);
constexpr size_t kFloatsPerVertex = 3lu + kNumColorStops;
constexpr size_t kDataStride = kFloatsPerVertex * sizeof(float);
constexpr size_t kGroundWeightOffset = 3 * sizeof(float);
constexpr size_t kMidGroundWeightOffset = 4 * sizeof(float);
constexpr size_t kBelowHorizonWeightOffset = 5 * sizeof(float);
constexpr size_t kHorizonWeightOffset = 6 * sizeof(float);
constexpr size_t kAboveHorizonWeightOffset = 7 * sizeof(float);
constexpr size_t kMidSkyWeightOffset = 8 * sizeof(float);
constexpr size_t kSkyWeightOffset = 9 * sizeof(float);
constexpr size_t kNumVertices = 2lu + kLinesLongitude * (kLinesLatitude - 2);
constexpr size_t kNumTriangles = 2lu * kLinesLongitude * (kLinesLatitude - 2);

constexpr float kSkyDistance = 6.0f;

float g_vertices[kNumVertices * kFloatsPerVertex];
GLushort g_indices[kNumTriangles * 3lu];

static constexpr char const* kVertexShader = SHADER(
  precision mediump float;
  uniform mat4 u_ModelViewProjMatrix;
  attribute vec4 a_Position;
  attribute float a_GroundWeight;
  attribute float a_MidGroundWeight;
  attribute float a_BelowHorizonWeight;
  attribute float a_HorizonWeight;
  attribute float a_AboveHorizonWeight;
  attribute float a_MidSkyWeight;
  attribute float a_SkyWeight;

  varying vec4 v_Position;
  varying float v_GroundWeight;
  varying float v_MidGroundWeight;
  varying float v_BelowHorizonWeight;
  varying float v_HorizonWeight;
  varying float v_AboveHorizonWeight;
  varying float v_MidSkyWeight;
  varying float v_SkyWeight;

  void main() {
    v_GroundWeight = a_GroundWeight;
    v_MidGroundWeight = a_MidGroundWeight;
    v_BelowHorizonWeight = a_BelowHorizonWeight;
    v_HorizonWeight = a_HorizonWeight;
    v_AboveHorizonWeight = a_AboveHorizonWeight;
    v_MidSkyWeight = a_MidSkyWeight;
    v_SkyWeight = a_SkyWeight;
    v_Position = a_Position;
    gl_Position = u_ModelViewProjMatrix * a_Position;
  }
);

static constexpr char const* kFragmentShader = SHADER(
  precision highp float;
  varying vec4 v_Position;
  varying float v_GroundWeight;
  varying float v_MidGroundWeight;
  varying float v_BelowHorizonWeight;
  varying float v_HorizonWeight;
  varying float v_AboveHorizonWeight;
  varying float v_MidSkyWeight;
  varying float v_SkyWeight;

  uniform vec3 u_GroundColor;
  uniform vec3 u_MidGroundColor;
  uniform vec3 u_BelowHorizonColor;
  uniform vec3 u_HorizonColor;
  uniform vec3 u_AboveHorizonColor;
  uniform vec3 u_MidSkyColor;
  uniform vec3 u_SkyColor;

  uniform float u_Opacity;

  void main() {
    vec3 color = v_GroundWeight * u_GroundColor
        + v_MidGroundWeight * u_MidGroundColor
        + v_BelowHorizonWeight * u_BelowHorizonColor
        + v_HorizonWeight * u_HorizonColor
        + v_AboveHorizonWeight * u_AboveHorizonColor
        + v_MidSkyWeight * u_MidSkyColor
        + v_SkyWeight * u_SkyColor;

    // Add some noise to prevent banding artifacts in the gradient.
    float n = (fract(dot(v_Position.xy, vec2(12345.67, 456.7))) - 0.5) / 255.0;
    gl_FragColor = vec4(color, 1.0) * u_Opacity + n;
  }
);
// clang-format on

void SetFloatAttribute(GLuint handle, size_t offset) {
  glVertexAttribPointer(handle, 1, GL_FLOAT, false, kDataStride,
                        VOID_OFFSET(offset));
  glEnableVertexAttribArray(handle);
}

}  // namespace

Sky::Sky() {
  animation_player().SetTransitionedProperties(
      {GRADIENT_GROUND_COLOR, GRADIENT_MID_GROUND_COLOR,
       GRADIENT_BELOW_HORIZON_COLOR, GRADIENT_HORIZON_COLOR,
       GRADIENT_ABOVE_HORIZON_COLOR, GRADIENT_MID_SKY_COLOR,
       GRADIENT_SKY_COLOR});
  set_hit_testable(false);
}

Sky::~Sky() = default;

void Sky::Render(UiElementRenderer* renderer, const CameraModel& camera) const {
  renderer->DrawSky(camera.view_proj_matrix * world_space_transform(), colors_,
                    computed_opacity());
}

// clang-format off
void Sky::SetColors(const SkyGradientColors& colors) {
  animation_player().TransitionColorTo(
      last_frame_time(), GRADIENT_GROUND_COLOR, colors_.ground, colors.ground);
  animation_player().TransitionColorTo(
      last_frame_time(), GRADIENT_MID_GROUND_COLOR, colors_.mid_ground,
      colors.mid_ground);
  animation_player().TransitionColorTo(
      last_frame_time(), GRADIENT_BELOW_HORIZON_COLOR, colors_.below_horizon,
      colors.below_horizon);
  animation_player().TransitionColorTo(
      last_frame_time(), GRADIENT_HORIZON_COLOR, colors_.horizon,
      colors.horizon);
  animation_player().TransitionColorTo(
      last_frame_time(), GRADIENT_ABOVE_HORIZON_COLOR, colors_.above_horizon,
      colors.above_horizon);
  animation_player().TransitionColorTo(
      last_frame_time(), GRADIENT_MID_SKY_COLOR, colors_.mid_sky,
      colors.mid_sky);
  animation_player().TransitionColorTo(
      last_frame_time(), GRADIENT_SKY_COLOR, colors_.sky, colors.sky);
}
// clang-format on

void Sky::NotifyClientColorAnimated(SkColor color,
                                    int target_property_id,
                                    cc::Animation* animation) {
  switch (target_property_id) {
    case GRADIENT_GROUND_COLOR:
      colors_.ground = color;
      break;
    case GRADIENT_MID_GROUND_COLOR:
      colors_.mid_ground = color;
      break;
    case GRADIENT_BELOW_HORIZON_COLOR:
      colors_.below_horizon = color;
      break;
    case GRADIENT_HORIZON_COLOR:
      colors_.horizon = color;
      break;
    case GRADIENT_ABOVE_HORIZON_COLOR:
      colors_.above_horizon = color;
      break;
    case GRADIENT_MID_SKY_COLOR:
      colors_.mid_sky = color;
      break;
    case GRADIENT_SKY_COLOR:
      colors_.sky = color;
      break;
    default:
      UiElement::NotifyClientColorAnimated(color, target_property_id,
                                           animation);
      break;
  }
}

Sky::Renderer::Renderer() : BaseRenderer(kVertexShader, kFragmentShader) {
  model_view_proj_matrix_handle_ =
      glGetUniformLocation(program_handle_, "u_ModelViewProjMatrix");
  ground_color_handle_ = glGetUniformLocation(program_handle_, "u_GroundColor");
  mid_ground_color_handle_ =
      glGetUniformLocation(program_handle_, "u_MidGroundColor");
  below_horizon_color_handle_ =
      glGetUniformLocation(program_handle_, "u_BelowHorizonColor");
  horizon_color_handle_ =
      glGetUniformLocation(program_handle_, "u_HorizonColor");
  above_horizon_color_handle_ =
      glGetUniformLocation(program_handle_, "u_AboveHorizonColor");
  mid_sky_color_handle_ =
      glGetUniformLocation(program_handle_, "u_MidSkyColor");
  sky_color_handle_ = glGetUniformLocation(program_handle_, "u_SkyColor");
  opacity_handle_ = glGetUniformLocation(program_handle_, "u_Opacity");

  ground_weight_handle_ =
      glGetAttribLocation(program_handle_, "a_GroundWeight");
  mid_ground_weight_handle_ =
      glGetAttribLocation(program_handle_, "a_MidGroundWeight");
  below_horizon_weight_handle_ =
      glGetAttribLocation(program_handle_, "a_BelowHorizonWeight");
  horizon_weight_handle_ =
      glGetAttribLocation(program_handle_, "a_HorizonWeight");
  above_horizon_weight_handle_ =
      glGetAttribLocation(program_handle_, "a_AboveHorizonWeight");
  mid_sky_weight_handle_ =
      glGetAttribLocation(program_handle_, "a_MidSkyWeight");
  sky_weight_handle_ = glGetAttribLocation(program_handle_, "a_SkyWeight");
}

Sky::Renderer::~Renderer() {}

void Sky::Renderer::Draw(const gfx::Transform& view_proj_matrix,
                         const SkyGradientColors& colors,
                         float opacity) {
  glUseProgram(program_handle_);

  // Pass in model view project matrix.
  glUniformMatrix4fv(model_view_proj_matrix_handle_, 1, false,
                     MatrixToGLArray(view_proj_matrix).data());

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);

  // Set up uniforms.
  SetOpaqueColorUniform(ground_color_handle_, colors.ground);
  SetOpaqueColorUniform(mid_ground_color_handle_, colors.mid_ground);
  SetOpaqueColorUniform(below_horizon_color_handle_, colors.below_horizon);
  SetOpaqueColorUniform(horizon_color_handle_, colors.horizon);
  SetOpaqueColorUniform(above_horizon_color_handle_, colors.above_horizon);
  SetOpaqueColorUniform(mid_sky_color_handle_, colors.mid_sky);
  SetOpaqueColorUniform(sky_color_handle_, colors.sky);
  glUniform1f(opacity_handle_, opacity);

  // Set up position attribute.
  glVertexAttribPointer(position_handle_, 3, GL_FLOAT, false, kDataStride,
                        VOID_OFFSET(0));
  glEnableVertexAttribArray(position_handle_);

  // Set up weight attributes.
  SetFloatAttribute(ground_weight_handle_, kGroundWeightOffset);
  SetFloatAttribute(mid_ground_weight_handle_, kMidGroundWeightOffset);
  SetFloatAttribute(below_horizon_weight_handle_, kBelowHorizonWeightOffset);
  SetFloatAttribute(horizon_weight_handle_, kHorizonWeightOffset);
  SetFloatAttribute(above_horizon_weight_handle_, kAboveHorizonWeightOffset);
  SetFloatAttribute(mid_sky_weight_handle_, kMidSkyWeightOffset);
  SetFloatAttribute(sky_weight_handle_, kSkyWeightOffset);

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_);

  glDrawElements(GL_TRIANGLES, arraysize(g_indices), GL_UNSIGNED_SHORT, 0);

  glDisableVertexAttribArray(position_handle_);
  glDisableVertexAttribArray(ground_color_handle_);
  glDisableVertexAttribArray(mid_ground_color_handle_);
  glDisableVertexAttribArray(below_horizon_color_handle_);
  glDisableVertexAttribArray(horizon_color_handle_);
  glDisableVertexAttribArray(above_horizon_color_handle_);
  glDisableVertexAttribArray(mid_sky_color_handle_);
  glDisableVertexAttribArray(sky_color_handle_);
}

GLuint Sky::Renderer::vertex_buffer_ = 0;
GLuint Sky::Renderer::index_buffer_ = 0;

void AddVertex(size_t* i,
               const gfx::Point3F& p,
               const cc::KeyframedFloatAnimationCurve& curve,
               float t) {
  g_vertices[*i * kFloatsPerVertex] = p.x();
  g_vertices[*i * kFloatsPerVertex + 1] = p.y();
  g_vertices[*i * kFloatsPerVertex + 2] = p.z();

  float f = curve.GetValue(base::TimeDelta::FromSecondsD(t));
  for (size_t j = 0; j < kNumColorStops; ++j) {
    g_vertices[*i * kFloatsPerVertex + 3 + j] =
        std::max(1.0f - std::abs(j - f), 0.0f);
  }

  (*i)++;
}

void Sky::Renderer::CreateBuffers() {
  auto curve = cc::KeyframedFloatAnimationCurve::Create();
  for (size_t i = 0; i < kNumColorStops; ++i) {
    curve->AddKeyframe(cc::FloatKeyframe::Create(
        base::TimeDelta::FromSecondsD(kColorStopTimeFractions[i]),
        static_cast<float>(i), nullptr));
  }

  size_t cur_vertex = 0lu;
  // Create special top / bottom vertices.
  AddVertex(&cur_vertex, {0.0f, -kSkyDistance, 0.0f}, *curve, 0.0f);
  AddVertex(&cur_vertex, {0.0f, kSkyDistance, 0.0f}, *curve, 1.0f);

  for (size_t latitude = 1; latitude + 1 < kLinesLatitude; ++latitude) {
    for (size_t longitude = 0; longitude < kLinesLongitude; ++longitude) {
      float t = static_cast<float>(latitude) / kLinesLatitude;
      float u = static_cast<float>(longitude) / kLinesLongitude;
      gfx::Transform local;
      local.RotateAboutYAxis(gfx::Tween::FloatValueBetween(u, 0.0f, 360.0f));

      // We want more precision near 0.5, so we'll apply a mapping that results
      // in a concentration of lines of latitude around the horizon.
      t = t * 2.0f - 1.0f;
      t = t * t * t;
      t = t * 0.5f + 0.5f;
      local.RotateAboutXAxis(gfx::Tween::FloatValueBetween(t, -90.0f, 90.0f));
      local.Translate3d({0, 0, -kSkyDistance});
      gfx::Point3F p;
      local.TransformPoint(&p);
      AddVertex(&cur_vertex, p, *curve, t);
    }
  }

  size_t cur_index = 0lu;
  size_t first_index_on_lowest_latitude = 2lu;
  size_t first_index_on_highest_latitude = kNumVertices - kLinesLongitude;
  for (size_t i = 0; i < kLinesLongitude; ++i) {
    size_t i_next = (i + 1) % kLinesLongitude;
    // Triangles to attach to the poles.
    g_indices[cur_index++] = 0;
    g_indices[cur_index++] = i + first_index_on_lowest_latitude;
    g_indices[cur_index++] = i_next + first_index_on_lowest_latitude;
    g_indices[cur_index++] = 1;
    g_indices[cur_index++] = i_next + first_index_on_highest_latitude;
    g_indices[cur_index++] = i + first_index_on_highest_latitude;
    for (size_t j = 1; j + 2 < kLinesLatitude; j++) {
      size_t j_next = j + 1;
      g_indices[cur_index++] =
          first_index_on_lowest_latitude + (j - 1) * kLinesLongitude + i;
      g_indices[cur_index++] =
          first_index_on_lowest_latitude + (j_next - 1) * kLinesLongitude + i;
      g_indices[cur_index++] =
          first_index_on_lowest_latitude + (j - 1) * kLinesLongitude + i_next;

      g_indices[cur_index++] = first_index_on_lowest_latitude +
                               (j_next - 1) * kLinesLongitude + i_next;
      g_indices[cur_index++] =
          first_index_on_lowest_latitude + (j - 1) * kLinesLongitude + i_next;
      g_indices[cur_index++] =
          first_index_on_lowest_latitude + (j_next - 1) * kLinesLongitude + i;
    }
  }

  GLuint buffers[2];
  glGenBuffersARB(2, buffers);
  vertex_buffer_ = buffers[0];
  index_buffer_ = buffers[1];

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
  glBufferData(GL_ARRAY_BUFFER, arraysize(g_vertices) * sizeof(float),
               g_vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, arraysize(g_indices) * sizeof(GLushort),
               g_indices, GL_STATIC_DRAW);
}

}  // namespace vr
