// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/message_loop/message_loop.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/point.h"
#include "ui/ozone/public/interfaces/overlay_surface_candidate_traits_test_service.mojom.h"
#include "ui/ozone/public/overlay_surface_candidate.h"

namespace ui {

namespace {

class OverlaySurfaceCandidateStructTraitsTest
    : public testing::Test,
      public ozone::mojom::OverlaySurfaceCandidateStructTraitsTestService {
 public:
  OverlaySurfaceCandidateStructTraitsTest() {}

 protected:
  ozone::mojom::OverlaySurfaceCandidateStructTraitsTestServicePtr
  GetTraitsTestProxy() {
    ozone::mojom::OverlaySurfaceCandidateStructTraitsTestServicePtr proxy;
    traits_test_bindings_.AddBinding(this, mojo::MakeRequest(&proxy));
    return proxy;
  }

 private:
  // OverlaySurfaceCandidateStructTraitsTestService:
  void EchoOverlaySurfaceCandidate(
      const OverlaySurfaceCandidate& p,
      EchoOverlaySurfaceCandidateCallback callback) override {
    std::move(callback).Run(p);
  }

  base::MessageLoop loop_;
  mojo::BindingSet<OverlaySurfaceCandidateStructTraitsTestService>
      traits_test_bindings_;

  DISALLOW_COPY_AND_ASSIGN(OverlaySurfaceCandidateStructTraitsTest);
};

}  // namespace

TEST_F(OverlaySurfaceCandidateStructTraitsTest, FieldsEqual) {
  ui::OverlaySurfaceCandidate input;

  input.transform = gfx::OVERLAY_TRANSFORM_FLIP_HORIZONTAL;
  input.format = gfx::BufferFormat::YUV_420_BIPLANAR;
  input.buffer_size = gfx::Size(6, 7);
  input.display_rect = gfx::RectF(1., 2., 3., 4.);
  input.crop_rect = gfx::RectF(10., 20., 30., 40.);
  input.quad_rect_in_target_space = gfx::Rect(101, 201, 301, 401);
  input.clip_rect = gfx::Rect(11, 21, 31, 41);
  input.is_clipped = true;
  input.plane_z_order = 42;
  input.overlay_handled = true;

  ui::OverlaySurfaceCandidate output(input);

  ui::ozone::mojom::OverlaySurfaceCandidateStructTraitsTestServicePtr proxy =
      GetTraitsTestProxy();
  proxy->EchoOverlaySurfaceCandidate(input, &output);

  EXPECT_EQ(input.transform, output.transform);
  EXPECT_EQ(input.format, output.format);
  EXPECT_EQ(input.buffer_size, output.buffer_size);
  EXPECT_EQ(input.display_rect, output.display_rect);
  EXPECT_EQ(input.crop_rect, output.crop_rect);
  EXPECT_EQ(input.quad_rect_in_target_space, output.quad_rect_in_target_space);
  EXPECT_EQ(input.clip_rect, output.clip_rect);
  EXPECT_EQ(input.is_clipped, output.is_clipped);
  EXPECT_EQ(input.plane_z_order, output.plane_z_order);
  EXPECT_EQ(input.overlay_handled, output.overlay_handled);
}

TEST_F(OverlaySurfaceCandidateStructTraitsTest, FalseBools) {
  ui::OverlaySurfaceCandidate input;

  input.is_clipped = false;
  input.overlay_handled = false;

  ui::OverlaySurfaceCandidate output(input);

  ui::ozone::mojom::OverlaySurfaceCandidateStructTraitsTestServicePtr proxy =
      GetTraitsTestProxy();
  proxy->EchoOverlaySurfaceCandidate(input, &output);

  EXPECT_EQ(input.is_clipped, output.is_clipped);
  EXPECT_EQ(input.overlay_handled, output.overlay_handled);
}

}  // namespace ui
