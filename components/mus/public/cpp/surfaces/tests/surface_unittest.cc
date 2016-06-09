// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <utility>

#include "cc/quads/debug_border_draw_quad.h"
#include "cc/quads/render_pass.h"
#include "cc/quads/solid_color_draw_quad.h"
#include "cc/quads/surface_draw_quad.h"
#include "cc/quads/texture_draw_quad.h"
#include "cc/resources/resource_provider.h"
#include "components/mus/public/cpp/surfaces/surfaces_type_converters.h"
#include "gpu/command_buffer/common/mailbox.h"
#include "gpu/command_buffer/common/mailbox_holder.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkXfermode.h"

using cc::mojom::Color;
using cc::mojom::ColorPtr;
using cc::mojom::DebugBorderQuadState;
using cc::mojom::DebugBorderQuadStatePtr;
using cc::mojom::DrawQuad;
using cc::mojom::DrawQuadPtr;
using cc::mojom::RenderPass;
using cc::mojom::RenderPassPtr;
using cc::mojom::RenderPassQuadState;
using cc::mojom::RenderPassQuadStatePtr;
using cc::mojom::SolidColorQuadState;
using cc::mojom::SolidColorQuadStatePtr;
using cc::mojom::SurfaceQuadState;
using cc::mojom::SurfaceQuadStatePtr;
using cc::mojom::TextureQuadState;
using cc::mojom::TextureQuadStatePtr;
using cc::mojom::TileQuadState;
using cc::mojom::TileQuadStatePtr;
using cc::mojom::YUVColorSpace;
using cc::mojom::YUVVideoQuadState;
using cc::mojom::YUVVideoQuadStatePtr;
using mus::mojom::CompositorFrame;
using mus::mojom::CompositorFramePtr;

namespace mojo {
namespace {


TEST(SurfaceLibTest, Color) {
  SkColor arbitrary_color = SK_ColorMAGENTA;
  SkColor round_trip = Color::From(arbitrary_color).To<SkColor>();
  EXPECT_EQ(arbitrary_color, round_trip);
}

class SurfaceLibQuadTest : public testing::Test {
 public:
  SurfaceLibQuadTest()
      : rect(5, 7, 13, 19),
        opaque_rect(rect),
        visible_rect(9, 11, 5, 7),
        needs_blending(false) {
    pass = cc::RenderPass::Create();
    sqs = pass->CreateAndAppendSharedQuadState();
  }

 protected:
  gfx::Rect rect;
  gfx::Rect opaque_rect;
  gfx::Rect visible_rect;
  bool needs_blending;
  std::unique_ptr<cc::RenderPass> pass;
  cc::SharedQuadState* sqs;
};

TEST_F(SurfaceLibQuadTest, ColorQuad) {
  cc::SolidColorDrawQuad* color_quad =
      pass->CreateAndAppendDrawQuad<cc::SolidColorDrawQuad>();
  SkColor arbitrary_color = SK_ColorGREEN;
  bool force_anti_aliasing_off = true;
  color_quad->SetAll(sqs,
                     rect,
                     opaque_rect,
                     visible_rect,
                     needs_blending,
                     arbitrary_color,
                     force_anti_aliasing_off);

  DrawQuadPtr mus_quad = DrawQuad::From<cc::DrawQuad>(*color_quad);
  ASSERT_FALSE(mus_quad.is_null());
  EXPECT_EQ(cc::mojom::Material::SOLID_COLOR, mus_quad->material);
  EXPECT_EQ(rect, mus_quad->rect);
  EXPECT_EQ(opaque_rect, mus_quad->opaque_rect);
  EXPECT_EQ(visible_rect, mus_quad->visible_rect);
  EXPECT_EQ(needs_blending, mus_quad->needs_blending);
  ASSERT_TRUE(mus_quad->solid_color_quad_state);
  SolidColorQuadStatePtr& mus_color_state = mus_quad->solid_color_quad_state;
  EXPECT_TRUE(Color::From(arbitrary_color).Equals(mus_color_state->color));
  EXPECT_EQ(force_anti_aliasing_off, mus_color_state->force_anti_aliasing_off);
}

TEST_F(SurfaceLibQuadTest, SurfaceQuad) {
  cc::SurfaceDrawQuad* surface_quad =
      pass->CreateAndAppendDrawQuad<cc::SurfaceDrawQuad>();
  cc::SurfaceId arbitrary_id(0, 5, 0);
  surface_quad->SetAll(
      sqs, rect, opaque_rect, visible_rect, needs_blending, arbitrary_id);

  DrawQuadPtr mus_quad = DrawQuad::From<cc::DrawQuad>(*surface_quad);
  ASSERT_FALSE(mus_quad.is_null());
  EXPECT_EQ(cc::mojom::Material::SURFACE_CONTENT, mus_quad->material);
  ASSERT_TRUE(mus_quad->surface_quad_state);
  SurfaceQuadStatePtr& mus_surface_state = mus_quad->surface_quad_state;
  EXPECT_EQ(arbitrary_id, mus_surface_state->surface);
}

TEST_F(SurfaceLibQuadTest, TextureQuad) {
  cc::TextureDrawQuad* texture_quad =
      pass->CreateAndAppendDrawQuad<cc::TextureDrawQuad>();
  unsigned resource_id = 9;
  bool premultiplied_alpha = true;
  gfx::PointF uv_top_left(1.7f, 2.1f);
  gfx::PointF uv_bottom_right(-7.f, 16.3f);
  SkColor background_color = SK_ColorYELLOW;
  float vertex_opacity[4] = {0.1f, 0.5f, 0.4f, 0.8f};
  bool y_flipped = false;
  bool nearest_neighbor = false;
  bool secure_output_only = true;
  texture_quad->SetAll(sqs, rect, opaque_rect, visible_rect, needs_blending,
                       resource_id, gfx::Size(), premultiplied_alpha,
                       uv_top_left, uv_bottom_right, background_color,
                       vertex_opacity, y_flipped, nearest_neighbor,
                       secure_output_only);

  DrawQuadPtr mus_quad = DrawQuad::From<cc::DrawQuad>(*texture_quad);
  ASSERT_FALSE(mus_quad.is_null());
  EXPECT_EQ(cc::mojom::Material::TEXTURE_CONTENT, mus_quad->material);
  ASSERT_TRUE(mus_quad->texture_quad_state);
  TextureQuadStatePtr& mus_texture_state = mus_quad->texture_quad_state;
  EXPECT_EQ(resource_id, mus_texture_state->resource_id);
  EXPECT_EQ(premultiplied_alpha, mus_texture_state->premultiplied_alpha);
  EXPECT_EQ(uv_top_left, mus_texture_state->uv_top_left);
  EXPECT_EQ(uv_bottom_right, mus_texture_state->uv_bottom_right);
  EXPECT_TRUE(Color::From(background_color)
                  .Equals(mus_texture_state->background_color));
  for (size_t i = 0; i < 4; ++i) {
    EXPECT_EQ(vertex_opacity[i], mus_texture_state->vertex_opacity[i]) << i;
  }
  EXPECT_EQ(y_flipped, mus_texture_state->y_flipped);
  EXPECT_EQ(secure_output_only, mus_texture_state->secure_output_only);
}

TEST_F(SurfaceLibQuadTest, TextureQuadEmptyVertexOpacity) {
  DrawQuadPtr mus_texture_quad = DrawQuad::New();
  mus_texture_quad->material = cc::mojom::Material::TEXTURE_CONTENT;
  TextureQuadStatePtr mus_texture_state = TextureQuadState::New();
  mus_texture_state->background_color = Color::New();
  mus_texture_quad->texture_quad_state = std::move(mus_texture_state);
  RenderPassPtr mus_pass = RenderPass::New();
  mus_pass->id.layer_id = 1;
  mus_pass->id.index = 1u;
  mus_pass->quads.push_back(std::move(mus_texture_quad));
  mus_pass->shared_quad_states.push_back(cc::SharedQuadState());

  std::unique_ptr<cc::RenderPass> pass =
      mus_pass.To<std::unique_ptr<cc::RenderPass>>();

  EXPECT_FALSE(pass);
}

TEST_F(SurfaceLibQuadTest, TextureQuadEmptyBackgroundColor) {
  DrawQuadPtr mus_texture_quad = DrawQuad::New();
  mus_texture_quad->material = cc::mojom::Material::TEXTURE_CONTENT;
  TextureQuadStatePtr mus_texture_state = TextureQuadState::New();
  mus_texture_state->vertex_opacity = mojo::Array<float>::New(4);
  mus_texture_quad->texture_quad_state = std::move(mus_texture_state);
  RenderPassPtr mus_pass = RenderPass::New();
  mus_pass->id.layer_id = 1;
  mus_pass->id.index = 1u;
  mus_pass->quads.push_back(std::move(mus_texture_quad));
  mus_pass->shared_quad_states.push_back(cc::SharedQuadState());

  std::unique_ptr<cc::RenderPass> pass =
      mus_pass.To<std::unique_ptr<cc::RenderPass>>();
  EXPECT_FALSE(pass);
}

TEST(SurfaceLibTest, RenderPass) {
  std::unique_ptr<cc::RenderPass> pass = cc::RenderPass::Create();
  cc::RenderPassId pass_id(1, 6);
  gfx::Rect output_rect(4, 9, 13, 71);
  gfx::Rect damage_rect(9, 17, 41, 45);
  gfx::Transform transform_to_root_target;
  transform_to_root_target.Skew(0.0, 43.0);
  bool has_transparent_background = false;
  pass->SetAll(pass_id,
               output_rect,
               damage_rect,
               transform_to_root_target,
               has_transparent_background);

  gfx::Transform quad_to_target_transform;
  quad_to_target_transform.Scale3d(0.3f, 0.7f, 0.9f);
  gfx::Size quad_layer_bounds(57, 39);
  gfx::Rect visible_quad_layer_rect(3, 7, 28, 42);
  gfx::Rect clip_rect(9, 12, 21, 31);
  bool is_clipped = true;
  float opacity = 0.65f;
  int sorting_context_id = 13;
  ::SkXfermode::Mode blend_mode = ::SkXfermode::kSrcOver_Mode;
  cc::SharedQuadState* sqs = pass->CreateAndAppendSharedQuadState();
  sqs->SetAll(quad_to_target_transform, quad_layer_bounds,
              visible_quad_layer_rect, clip_rect, is_clipped, opacity,
              blend_mode, sorting_context_id);

  gfx::Rect rect(5, 7, 13, 19);
  gfx::Rect opaque_rect(rect);
  gfx::Rect visible_rect(9, 11, 5, 7);
  bool needs_blending = false;

  cc::SolidColorDrawQuad* color_quad =
      pass->CreateAndAppendDrawQuad<cc::SolidColorDrawQuad>();
  SkColor arbitrary_color = SK_ColorGREEN;
  bool force_anti_aliasing_off = true;
  color_quad->SetAll(pass->shared_quad_state_list.back(),
                     rect,
                     opaque_rect,
                     visible_rect,
                     needs_blending,
                     arbitrary_color,
                     force_anti_aliasing_off);

  cc::SurfaceDrawQuad* surface_quad =
      pass->CreateAndAppendDrawQuad<cc::SurfaceDrawQuad>();
  cc::SurfaceId arbitrary_id(0, 5, 0);
  surface_quad->SetAll(
      sqs, rect, opaque_rect, visible_rect, needs_blending, arbitrary_id);

  cc::TextureDrawQuad* texture_quad =
      pass->CreateAndAppendDrawQuad<cc::TextureDrawQuad>();
  unsigned resource_id = 9;
  bool premultiplied_alpha = true;
  gfx::PointF uv_top_left(1.7f, 2.1f);
  gfx::PointF uv_bottom_right(-7.f, 16.3f);
  SkColor background_color = SK_ColorYELLOW;
  float vertex_opacity[4] = {0.1f, 0.5f, 0.4f, 0.8f};
  bool y_flipped = false;
  bool nearest_neighbor = false;
  bool secure_output_only = false;
  texture_quad->SetAll(sqs, rect, opaque_rect, visible_rect, needs_blending,
                       resource_id, gfx::Size(), premultiplied_alpha,
                       uv_top_left, uv_bottom_right, background_color,
                       vertex_opacity, y_flipped, nearest_neighbor,
                       secure_output_only);

  RenderPassPtr mus_pass = RenderPass::From(*pass);
  ASSERT_FALSE(mus_pass.is_null());
  EXPECT_EQ(6u, mus_pass->id.index);
  EXPECT_EQ(output_rect, mus_pass->output_rect);
  EXPECT_EQ(damage_rect, mus_pass->damage_rect);
  EXPECT_EQ(transform_to_root_target, mus_pass->transform_to_root_target);
  EXPECT_EQ(has_transparent_background, mus_pass->has_transparent_background);
  ASSERT_EQ(1u, mus_pass->shared_quad_states.size());
  ASSERT_EQ(3u, mus_pass->quads.size());
  EXPECT_EQ(0u, mus_pass->quads[0]->shared_quad_state_index);

  std::unique_ptr<cc::RenderPass> round_trip_pass =
      mus_pass.To<std::unique_ptr<cc::RenderPass>>();
  EXPECT_EQ(pass_id, round_trip_pass->id);
  EXPECT_EQ(output_rect, round_trip_pass->output_rect);
  EXPECT_EQ(damage_rect, round_trip_pass->damage_rect);
  EXPECT_EQ(transform_to_root_target,
            round_trip_pass->transform_to_root_target);
  EXPECT_EQ(has_transparent_background,
            round_trip_pass->has_transparent_background);
  ASSERT_EQ(1u, round_trip_pass->shared_quad_state_list.size());
  ASSERT_EQ(3u, round_trip_pass->quad_list.size());
  EXPECT_EQ(round_trip_pass->shared_quad_state_list.front(),
            round_trip_pass->quad_list.front()->shared_quad_state);

  cc::SharedQuadState* round_trip_sqs =
      round_trip_pass->shared_quad_state_list.front();
  EXPECT_EQ(quad_to_target_transform, round_trip_sqs->quad_to_target_transform);
  EXPECT_EQ(quad_layer_bounds, round_trip_sqs->quad_layer_bounds);
  EXPECT_EQ(visible_quad_layer_rect, round_trip_sqs->visible_quad_layer_rect);
  EXPECT_EQ(clip_rect, round_trip_sqs->clip_rect);
  EXPECT_EQ(is_clipped, round_trip_sqs->is_clipped);
  EXPECT_EQ(opacity, round_trip_sqs->opacity);
  EXPECT_EQ(sorting_context_id, round_trip_sqs->sorting_context_id);

  cc::DrawQuad* round_trip_quad = round_trip_pass->quad_list.front();
  // First is solid color quad.
  ASSERT_EQ(cc::DrawQuad::SOLID_COLOR, round_trip_quad->material);
  EXPECT_EQ(rect, round_trip_quad->rect);
  EXPECT_EQ(opaque_rect, round_trip_quad->opaque_rect);
  EXPECT_EQ(visible_rect, round_trip_quad->visible_rect);
  EXPECT_EQ(needs_blending, round_trip_quad->needs_blending);
  const cc::SolidColorDrawQuad* round_trip_color_quad =
      cc::SolidColorDrawQuad::MaterialCast(round_trip_quad);
  EXPECT_EQ(arbitrary_color, round_trip_color_quad->color);
  EXPECT_EQ(force_anti_aliasing_off,
            round_trip_color_quad->force_anti_aliasing_off);

  round_trip_quad = round_trip_pass->quad_list.ElementAt(1);
  // Second is surface quad.
  ASSERT_EQ(cc::DrawQuad::SURFACE_CONTENT, round_trip_quad->material);
  const cc::SurfaceDrawQuad* round_trip_surface_quad =
      cc::SurfaceDrawQuad::MaterialCast(round_trip_quad);
  EXPECT_EQ(arbitrary_id, round_trip_surface_quad->surface_id);

  round_trip_quad = round_trip_pass->quad_list.ElementAt(2);
  // Third is texture quad.
  ASSERT_EQ(cc::DrawQuad::TEXTURE_CONTENT, round_trip_quad->material);
  const cc::TextureDrawQuad* round_trip_texture_quad =
      cc::TextureDrawQuad::MaterialCast(round_trip_quad);
  EXPECT_EQ(resource_id, round_trip_texture_quad->resource_id());
  EXPECT_EQ(premultiplied_alpha, round_trip_texture_quad->premultiplied_alpha);
  EXPECT_EQ(uv_top_left, round_trip_texture_quad->uv_top_left);
  EXPECT_EQ(uv_bottom_right, round_trip_texture_quad->uv_bottom_right);
  EXPECT_EQ(background_color, round_trip_texture_quad->background_color);
  for (size_t i = 0; i < 4; ++i) {
    EXPECT_EQ(vertex_opacity[i], round_trip_texture_quad->vertex_opacity[i])
        << i;
  }
  EXPECT_EQ(y_flipped, round_trip_texture_quad->y_flipped);
  EXPECT_EQ(secure_output_only, round_trip_texture_quad->secure_output_only);
}

TEST_F(SurfaceLibQuadTest, DebugBorderQuad) {
  cc::DebugBorderDrawQuad* debug_border_quad =
      pass->CreateAndAppendDrawQuad<cc::DebugBorderDrawQuad>();
  const SkColor arbitrary_color = SK_ColorGREEN;
  const int width = 3;
  debug_border_quad->SetAll(sqs,
                            rect,
                            opaque_rect,
                            visible_rect,
                            needs_blending,
                            arbitrary_color,
                            width);

  DrawQuadPtr mus_quad = DrawQuad::From<cc::DrawQuad>(*debug_border_quad);
  ASSERT_FALSE(mus_quad.is_null());
  EXPECT_EQ(cc::mojom::Material::DEBUG_BORDER, mus_quad->material);
  EXPECT_EQ(rect, mus_quad->rect);
  EXPECT_EQ(opaque_rect, mus_quad->opaque_rect);
  EXPECT_EQ(visible_rect, mus_quad->visible_rect);
  EXPECT_EQ(needs_blending, mus_quad->needs_blending);
  ASSERT_TRUE(mus_quad->debug_border_quad_state);
  DebugBorderQuadStatePtr& mus_debug_border_state =
      mus_quad->debug_border_quad_state;
  EXPECT_TRUE(
      Color::From(arbitrary_color).Equals(mus_debug_border_state->color));
  EXPECT_EQ(width, mus_debug_border_state->width);
}

}  // namespace
}  // namespace mojo
