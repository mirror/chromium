// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/host/hit_test/hit_test_query.h"

#include <cstdint>

#include "services/viz/public/interfaces/hit_test/hit_test_region_list.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace viz {
namespace test {

class HitTestQueryTest : public testing::Test {
 public:
  HitTestQueryTest() = default;
  ~HitTestQueryTest() override = default;

 protected:
  HitTestQuery& hit_test_query() { return hit_test_query_; }
  AggregatedHitTestRegion* aggregated_hit_test_region() {
    return static_cast<AggregatedHitTestRegion*>(active_buffer_.get());
  }

 private:
  // testing::Test:
  void SetUp() override {
    uint32_t handle_size = 100;
    size_t num_bytes = handle_size * sizeof(AggregatedHitTestRegion);
    mojo::ScopedSharedBufferHandle active_handle =
        mojo::SharedBufferHandle::Create(num_bytes);
    mojo::ScopedSharedBufferHandle idle_handle =
        mojo::SharedBufferHandle::Create(num_bytes);
    active_buffer_ = active_handle->Map(num_bytes);
    hit_test_query_.OnAggregatedHitTestRegionListUpdated(
        std::move(active_handle), handle_size, std::move(idle_handle),
        handle_size);
  }
  void TearDown() override {}

  HitTestQuery hit_test_query_;
  mojo::ScopedSharedBufferMapping active_buffer_;

  DISALLOW_COPY_AND_ASSIGN(HitTestQueryTest);
};

// One surface.
//
//  +e---------+
//  |          |
//  |          |
//  |          |
//  +----------+
//
TEST_F(HitTestQueryTest, OneSurface) {
  FrameSinkId e_id = FrameSinkId(1, 1);
  gfx::Rect e_bounds = gfx::Rect(0, 0, 600, 600);
  gfx::Transform transform_e_to_e;
  AggregatedHitTestRegion* aggregated_hit_test_region_list =
      aggregated_hit_test_region();

  // e
  aggregated_hit_test_region_list[0].frame_sink_id = e_id;
  aggregated_hit_test_region_list[0].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[0].transform = transform_e_to_e;
  aggregated_hit_test_region_list[0].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, e_bounds));
  aggregated_hit_test_region_list[0].child_count = 0;

  // All points are in e's coordinate system when we reach this case.
  gfx::Point point1(1, 1);
  gfx::Point point2(600, 600);
  gfx::Point point3(0, 0);

  Target target1 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point1);
  EXPECT_EQ(target1.frame_sink_id, e_id);
  EXPECT_EQ(target1.location_in_target, point1);
  EXPECT_EQ(target1.flags, mojom::kHitTestMine);

  // point2 is on the bounds of e so no target found.
  Target target2 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point2);
  EXPECT_EQ(target2.frame_sink_id, FrameSinkId());
  EXPECT_EQ(target2.location_in_target, gfx::Point());
  EXPECT_FALSE(target2.flags);

  // There's a valid Target for point3, see Rect::Contains.
  Target target3 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point3);
  EXPECT_EQ(target3.frame_sink_id, e_id);
  EXPECT_EQ(target3.location_in_target, point3);
  EXPECT_EQ(target3.flags, mojom::kHitTestMine);
}

// One embedder with two children.
//
//  +e------------+     Point   maps to
//  | +c1-+ +c2---|     -----   -------
//  |1|   | |     |      1        e
//  | | 2 | | 3   | 4    2        c1
//  | +---+ |     |      3        c2
//  +-------------+      4        none
//
TEST_F(HitTestQueryTest, OneEmbedderTwoChildren) {
  FrameSinkId e_id = FrameSinkId(1, 1);
  FrameSinkId c1_id = FrameSinkId(2, 2);
  FrameSinkId c2_id = FrameSinkId(3, 3);
  gfx::Rect e_bounds_in_e = gfx::Rect(0, 0, 600, 600);
  gfx::Rect c1_bounds_in_e = gfx::Rect(0, 0, 200, 200);
  gfx::Rect c2_bounds_in_e = gfx::Rect(0, 0, 400, 400);
  gfx::Transform transform_e_to_e, transform_e_to_c1, transform_e_to_c2;
  transform_e_to_c1.Translate(-100, -100);
  transform_e_to_c2.Translate(-300, -300);
  AggregatedHitTestRegion* aggregated_hit_test_region_list =
      aggregated_hit_test_region();

  // e
  aggregated_hit_test_region_list[0].frame_sink_id = e_id;
  aggregated_hit_test_region_list[0].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[0].transform = transform_e_to_e;
  aggregated_hit_test_region_list[0].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, e_bounds_in_e));
  aggregated_hit_test_region_list[0].child_count = 2;

  // c1
  aggregated_hit_test_region_list[1].frame_sink_id = c1_id;
  aggregated_hit_test_region_list[1].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[1].transform = transform_e_to_c1;
  aggregated_hit_test_region_list[1].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, c1_bounds_in_e));
  aggregated_hit_test_region_list[1].child_count = 0;

  // c2
  aggregated_hit_test_region_list[2].frame_sink_id = c2_id;
  aggregated_hit_test_region_list[2].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[2].transform = transform_e_to_c2;
  aggregated_hit_test_region_list[2].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, c2_bounds_in_e));
  aggregated_hit_test_region_list[2].child_count = 0;

  // All points are in e's coordinate system when we reach this case.
  gfx::Point point1(99, 200);
  gfx::Point point2(150, 150);
  gfx::Point point3(400, 400);
  gfx::Point point4(650, 350);

  Target target1 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point1);
  EXPECT_EQ(target1.frame_sink_id, e_id);
  EXPECT_EQ(target1.location_in_target, point1);
  EXPECT_EQ(target1.flags, mojom::kHitTestMine);

  Target target2 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point2);
  EXPECT_EQ(target2.frame_sink_id, c1_id);
  EXPECT_EQ(target2.location_in_target, gfx::Point(50, 50));
  EXPECT_EQ(target2.flags, mojom::kHitTestMine);

  Target target3 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point3);
  EXPECT_EQ(target3.frame_sink_id, c2_id);
  EXPECT_EQ(target3.location_in_target, gfx::Point(100, 100));
  EXPECT_EQ(target3.flags, mojom::kHitTestMine);

  Target target4 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point4);
  EXPECT_EQ(target4.frame_sink_id, FrameSinkId());
  EXPECT_EQ(target4.location_in_target, gfx::Point());
  EXPECT_FALSE(target4.flags);
}

// One embedder with a rotated child.
TEST_F(HitTestQueryTest, OneEmbedderRotatedChild) {
  FrameSinkId e_id = FrameSinkId(1, 1);
  FrameSinkId c_id = FrameSinkId(2, 2);
  gfx::Rect e_bounds_in_e = gfx::Rect(0, 0, 600, 600);
  gfx::Rect c_bounds_in_e = gfx::Rect(0, 0, 1000, 1000);
  gfx::Transform transform_e_to_e, transform_e_to_c;
  transform_e_to_c.Translate(-100, -100);
  transform_e_to_c.Skew(2, 3);
  transform_e_to_c.Scale(.5f, .7f);
  AggregatedHitTestRegion* aggregated_hit_test_region_list =
      aggregated_hit_test_region();

  // e
  aggregated_hit_test_region_list[0].frame_sink_id = e_id;
  aggregated_hit_test_region_list[0].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[0].transform = transform_e_to_e;
  aggregated_hit_test_region_list[0].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, e_bounds_in_e));
  aggregated_hit_test_region_list[0].child_count = 1;

  // c
  aggregated_hit_test_region_list[1].frame_sink_id = c_id;
  aggregated_hit_test_region_list[1].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[1].transform = transform_e_to_c;
  aggregated_hit_test_region_list[1].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, c_bounds_in_e));
  aggregated_hit_test_region_list[1].child_count = 0;

  // All points are in e's coordinate system when we reach this case.
  gfx::Point point1(150, 120);  // Point(-22, -12) after transform.
  gfx::Point point2(550, 400);  // Point(185, 194) after transform.

  Target target1 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point1);
  EXPECT_EQ(target1.frame_sink_id, e_id);
  EXPECT_EQ(target1.location_in_target, point1);
  EXPECT_EQ(target1.flags, mojom::kHitTestMine);

  Target target2 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point2);
  EXPECT_EQ(target2.frame_sink_id, c_id);
  EXPECT_EQ(target2.location_in_target, gfx::Point(185, 194));
  EXPECT_EQ(target2.flags, mojom::kHitTestMine);
}

// One embedder with a clipped child with a tab and transparent background.
//
//  +e-------------+
//  |   +c---------|     Point   maps to
//  | 1 |+a--+     |     -----   -------
//  |   || 2 |  3  |       1        e
//  |   |+b--------|       2        a
//  |   ||         |       3        e ( transparent area in c )
//  |   ||   4     |       4        b
//  +--------------+
//
TEST_F(HitTestQueryTest, ClippedChildWithTabAndTransparentBackground) {
  FrameSinkId e_id = FrameSinkId(1, 1);
  FrameSinkId c_id = FrameSinkId(2, 2);
  FrameSinkId a_id = FrameSinkId(3, 3);
  FrameSinkId b_id = FrameSinkId(4, 4);
  gfx::Rect e_bounds_in_e = gfx::Rect(0, 0, 600, 600);
  gfx::Rect c_bounds_in_e = gfx::Rect(0, 0, 800, 800);
  gfx::Rect a_bounds_in_c = gfx::Rect(0, 0, 200, 100);
  gfx::Rect b_bounds_in_c = gfx::Rect(0, 0, 800, 600);
  gfx::Transform transform_e_to_e, transform_e_to_c, transform_c_to_a,
      transform_c_to_b;
  transform_e_to_c.Translate(-200, -100);
  transform_c_to_b.Translate(0, -100);
  AggregatedHitTestRegion* aggregated_hit_test_region_list =
      aggregated_hit_test_region();

  // e
  aggregated_hit_test_region_list[0].frame_sink_id = e_id;
  aggregated_hit_test_region_list[0].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[0].transform = transform_e_to_e;
  aggregated_hit_test_region_list[0].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, e_bounds_in_e));
  aggregated_hit_test_region_list[0].child_count = 3;

  // c
  aggregated_hit_test_region_list[1].frame_sink_id = c_id;
  aggregated_hit_test_region_list[1].flags =
      mojom::kHitTestChildSurface | mojom::kHitTestIgnore;
  aggregated_hit_test_region_list[1].transform = transform_e_to_c;
  aggregated_hit_test_region_list[1].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, c_bounds_in_e));
  aggregated_hit_test_region_list[1].child_count = 2;

  // a
  aggregated_hit_test_region_list[2].frame_sink_id = a_id;
  aggregated_hit_test_region_list[2].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[2].transform = transform_c_to_a;
  aggregated_hit_test_region_list[2].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, a_bounds_in_c));
  aggregated_hit_test_region_list[2].child_count = 0;

  // b
  aggregated_hit_test_region_list[3].frame_sink_id = b_id;
  aggregated_hit_test_region_list[3].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[3].transform = transform_c_to_b;
  aggregated_hit_test_region_list[3].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, b_bounds_in_c));
  aggregated_hit_test_region_list[3].child_count = 0;

  // All points are in e's coordinate system when we reach this case.
  gfx::Point point1(1, 1);
  gfx::Point point2(202, 102);
  gfx::Point point3(403, 103);
  gfx::Point point4(202, 202);

  Target target1 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point1);
  EXPECT_EQ(target1.frame_sink_id, e_id);
  EXPECT_EQ(target1.location_in_target, point1);
  EXPECT_EQ(target1.flags, mojom::kHitTestMine);

  Target target2 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point2);
  EXPECT_EQ(target2.frame_sink_id, a_id);
  EXPECT_EQ(target2.location_in_target, gfx::Point(2, 2));
  EXPECT_EQ(target2.flags, mojom::kHitTestMine);

  Target target3 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point3);
  EXPECT_EQ(target3.frame_sink_id, e_id);
  EXPECT_EQ(target3.location_in_target, point3);
  EXPECT_EQ(target3.flags, mojom::kHitTestMine);

  Target target4 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point4);
  EXPECT_EQ(target4.frame_sink_id, b_id);
  EXPECT_EQ(target4.location_in_target, gfx::Point(2, 2));
  EXPECT_EQ(target4.flags, mojom::kHitTestMine);
}

// One embedder with a clipped child with a tab and transparent background,
// and a child d under that.
//
//  +e-------------+
//  |      +d------|
//  |   +c-|-------|     Point   maps to
//  | 1 |+a|-+     |     -----   -------
//  |   || 2 |  3  |       1        e
//  |   |+b|-------|       2        a
//  |   || |       |       3        d
//  |   || | 4     |       4        b
//  +--------------+
//
TEST_F(HitTestQueryTest, ClippedChildWithChildUnderneath) {
  FrameSinkId e_id = FrameSinkId(1, 1);
  FrameSinkId c_id = FrameSinkId(2, 2);
  FrameSinkId a_id = FrameSinkId(3, 3);
  FrameSinkId b_id = FrameSinkId(4, 4);
  FrameSinkId d_id = FrameSinkId(5, 5);
  gfx::Rect e_bounds_in_e = gfx::Rect(0, 0, 600, 600);
  gfx::Rect c_bounds_in_e = gfx::Rect(0, 0, 800, 800);
  gfx::Rect a_bounds_in_c = gfx::Rect(0, 0, 200, 100);
  gfx::Rect b_bounds_in_c = gfx::Rect(0, 0, 800, 600);
  gfx::Rect d_bounds_in_e = gfx::Rect(0, 0, 800, 800);
  gfx::Transform transform_e_to_e, transform_e_to_c, transform_c_to_a,
      transform_c_to_b, transform_e_to_d;
  transform_e_to_c.Translate(-200, -100);
  transform_c_to_b.Translate(0, -100);
  transform_e_to_d.Translate(-400, -50);
  AggregatedHitTestRegion* aggregated_hit_test_region_list =
      aggregated_hit_test_region();

  // e
  aggregated_hit_test_region_list[0].frame_sink_id = e_id;
  aggregated_hit_test_region_list[0].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[0].transform = transform_e_to_e;
  aggregated_hit_test_region_list[0].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, e_bounds_in_e));
  aggregated_hit_test_region_list[0].child_count = 4;

  // c
  aggregated_hit_test_region_list[1].frame_sink_id = c_id;
  aggregated_hit_test_region_list[1].flags =
      mojom::kHitTestChildSurface | mojom::kHitTestIgnore;
  aggregated_hit_test_region_list[1].transform = transform_e_to_c;
  aggregated_hit_test_region_list[1].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, c_bounds_in_e));
  aggregated_hit_test_region_list[1].child_count = 2;

  // a
  aggregated_hit_test_region_list[2].frame_sink_id = a_id;
  aggregated_hit_test_region_list[2].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[2].transform = transform_c_to_a;
  aggregated_hit_test_region_list[2].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, a_bounds_in_c));
  aggregated_hit_test_region_list[2].child_count = 0;

  // b
  aggregated_hit_test_region_list[3].frame_sink_id = b_id;
  aggregated_hit_test_region_list[3].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[3].transform = transform_c_to_b;
  aggregated_hit_test_region_list[3].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, b_bounds_in_c));
  aggregated_hit_test_region_list[3].child_count = 0;

  // d
  aggregated_hit_test_region_list[4].frame_sink_id = d_id;
  aggregated_hit_test_region_list[4].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[4].transform = transform_e_to_d;
  aggregated_hit_test_region_list[4].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, d_bounds_in_e));
  aggregated_hit_test_region_list[4].child_count = 0;

  // All points are in e's coordinate system when we reach this case.
  gfx::Point point1(1, 1);
  gfx::Point point2(202, 102);
  gfx::Point point3(450, 150);
  gfx::Point point4(202, 202);

  Target target1 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point1);
  EXPECT_EQ(target1.frame_sink_id, e_id);
  EXPECT_EQ(target1.location_in_target, point1);
  EXPECT_EQ(target1.flags, mojom::kHitTestMine);

  Target target2 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point2);
  EXPECT_EQ(target2.frame_sink_id, a_id);
  EXPECT_EQ(target2.location_in_target, gfx::Point(2, 2));
  EXPECT_EQ(target2.flags, mojom::kHitTestMine);

  Target target3 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point3);
  EXPECT_EQ(target3.frame_sink_id, d_id);
  EXPECT_EQ(target3.location_in_target, gfx::Point(50, 100));
  EXPECT_EQ(target3.flags, mojom::kHitTestMine);

  Target target4 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point4);
  EXPECT_EQ(target4.frame_sink_id, b_id);
  EXPECT_EQ(target4.location_in_target, gfx::Point(2, 2));
  EXPECT_EQ(target4.flags, mojom::kHitTestMine);
}

// Tests transforming location to be in target's coordinate system given the
// target's ancestor list, in the case of ClippedChildWithChildUnderneath test.
TEST_F(HitTestQueryTest, ClippedChildWithChildUnderneathTransform) {
  FrameSinkId e_id = FrameSinkId(1, 1);
  FrameSinkId c_id = FrameSinkId(2, 2);
  FrameSinkId a_id = FrameSinkId(3, 3);
  FrameSinkId b_id = FrameSinkId(4, 4);
  FrameSinkId d_id = FrameSinkId(5, 5);
  gfx::Rect e_bounds_in_e = gfx::Rect(0, 0, 600, 600);
  gfx::Rect c_bounds_in_e = gfx::Rect(0, 0, 800, 800);
  gfx::Rect a_bounds_in_c = gfx::Rect(0, 0, 200, 100);
  gfx::Rect b_bounds_in_c = gfx::Rect(0, 100, 800, 600);
  gfx::Rect d_bounds_in_e = gfx::Rect(0, 0, 800, 800);
  gfx::Transform transform_e_to_e, transform_e_to_c, transform_c_to_a,
      transform_c_to_b, transform_e_to_d;
  transform_e_to_c.Translate(-200, -100);
  transform_e_to_d.Translate(-400, -50);
  AggregatedHitTestRegion* aggregated_hit_test_region_list =
      aggregated_hit_test_region();

  // e
  aggregated_hit_test_region_list[0].frame_sink_id = e_id;
  aggregated_hit_test_region_list[0].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[0].transform = transform_e_to_e;
  aggregated_hit_test_region_list[0].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, e_bounds_in_e));
  aggregated_hit_test_region_list[0].child_count = 4;

  // c
  aggregated_hit_test_region_list[1].frame_sink_id = c_id;
  aggregated_hit_test_region_list[1].flags =
      mojom::kHitTestChildSurface | mojom::kHitTestIgnore;
  aggregated_hit_test_region_list[1].transform = transform_e_to_c;
  aggregated_hit_test_region_list[1].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, c_bounds_in_e));
  aggregated_hit_test_region_list[1].child_count = 2;

  // a
  aggregated_hit_test_region_list[2].frame_sink_id = a_id;
  aggregated_hit_test_region_list[2].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[2].transform = transform_c_to_a;
  aggregated_hit_test_region_list[2].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, a_bounds_in_c));
  aggregated_hit_test_region_list[2].child_count = 0;

  // b
  aggregated_hit_test_region_list[3].frame_sink_id = b_id;
  aggregated_hit_test_region_list[3].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[3].transform = transform_c_to_b;
  aggregated_hit_test_region_list[3].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, b_bounds_in_c));
  aggregated_hit_test_region_list[3].child_count = 0;

  // d
  aggregated_hit_test_region_list[4].frame_sink_id = d_id;
  aggregated_hit_test_region_list[4].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[4].transform = transform_e_to_d;
  aggregated_hit_test_region_list[4].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, d_bounds_in_e));
  aggregated_hit_test_region_list[4].child_count = 0;

  // All points are in e's coordinate system when we reach this case.
  gfx::Point point1(1, 1);
  gfx::Point point2(202, 102);
  gfx::Point point3(450, 150);
  gfx::Point point4(202, 202);

  std::vector<FrameSinkId> target_ancestors1{e_id};
  EXPECT_EQ(hit_test_query().TransformLocationForTarget(
                EventSource::MOUSE, target_ancestors1, point1),
            point1);
  std::vector<FrameSinkId> target_ancestors2{a_id, c_id, e_id};
  EXPECT_EQ(hit_test_query().TransformLocationForTarget(
                EventSource::MOUSE, target_ancestors2, point2),
            gfx::Point(2, 2));
  std::vector<FrameSinkId> target_ancestors3{d_id, e_id};
  EXPECT_EQ(hit_test_query().TransformLocationForTarget(
                EventSource::MOUSE, target_ancestors3, point3),
            gfx::Point(50, 100));
  std::vector<FrameSinkId> target_ancestors4{b_id, c_id, e_id};
  EXPECT_EQ(hit_test_query().TransformLocationForTarget(
                EventSource::MOUSE, target_ancestors4, point4),
            gfx::Point(2, 2));
}

// One embedder with two clipped children with a tab and transparent background.
//
//  +e-------------+
//  |   +c1--------|     Point   maps to
//  | 1 |+a--+     |     -----   -------
//  |   || 2 |  3  |       1        e
//  |   |+b--------|       2        a
//  |   ||         |       3        e ( transparent area in c1 )
//  |   ||   4     |       4        b
//  |   +----------|       5        g
//  |   +c2--------|       6        e ( transparent area in c2 )
//  |   |+g--+     |       7        h
//  |   || 5 |  6  |
//  |   |+h--------|
//  |   ||         |
//  |   ||   7     |
//  +--------------+
//
TEST_F(HitTestQueryTest, ClippedChildrenWithTabAndTransparentBackground) {
  FrameSinkId e_id = FrameSinkId(1, 1);
  FrameSinkId c1_id = FrameSinkId(2, 2);
  FrameSinkId a_id = FrameSinkId(3, 3);
  FrameSinkId b_id = FrameSinkId(4, 4);
  FrameSinkId c2_id = FrameSinkId(5, 5);
  FrameSinkId g_id = FrameSinkId(6, 6);
  FrameSinkId h_id = FrameSinkId(7, 7);
  gfx::Rect e_bounds_in_e = gfx::Rect(0, 0, 600, 1200);
  gfx::Rect c1_bounds_in_e = gfx::Rect(0, 0, 800, 500);
  gfx::Rect a_bounds_in_c1 = gfx::Rect(0, 0, 200, 100);
  gfx::Rect b_bounds_in_c1 = gfx::Rect(0, 0, 800, 400);
  gfx::Rect c2_bounds_in_e = gfx::Rect(0, 0, 800, 500);
  gfx::Rect g_bounds_in_c2 = gfx::Rect(0, 0, 200, 100);
  gfx::Rect h_bounds_in_c2 = gfx::Rect(0, 0, 800, 800);
  gfx::Transform transform_e_to_e, transform_e_to_c1, transform_c1_to_a,
      transform_c1_to_b, transform_e_to_c2, transform_c2_to_g,
      transform_c2_to_h;
  transform_e_to_c1.Translate(-200, -100);
  transform_c1_to_b.Translate(0, -100);
  transform_e_to_c2.Translate(-200, -700);
  transform_c2_to_h.Translate(0, -100);
  AggregatedHitTestRegion* aggregated_hit_test_region_list =
      aggregated_hit_test_region();

  // e
  aggregated_hit_test_region_list[0].frame_sink_id = e_id;
  aggregated_hit_test_region_list[0].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[0].transform = transform_e_to_e;
  aggregated_hit_test_region_list[0].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, e_bounds_in_e));
  aggregated_hit_test_region_list[0].child_count = 6;

  // c1
  aggregated_hit_test_region_list[1].frame_sink_id = c1_id;
  aggregated_hit_test_region_list[1].flags =
      mojom::kHitTestChildSurface | mojom::kHitTestIgnore;
  aggregated_hit_test_region_list[1].transform = transform_e_to_c1;
  aggregated_hit_test_region_list[1].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, c1_bounds_in_e));
  aggregated_hit_test_region_list[1].child_count = 2;

  // a
  aggregated_hit_test_region_list[2].frame_sink_id = a_id;
  aggregated_hit_test_region_list[2].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[2].transform = transform_c1_to_a;
  aggregated_hit_test_region_list[2].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, a_bounds_in_c1));
  aggregated_hit_test_region_list[2].child_count = 0;

  // b
  aggregated_hit_test_region_list[3].frame_sink_id = b_id;
  aggregated_hit_test_region_list[3].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[3].transform = transform_c1_to_b;
  aggregated_hit_test_region_list[3].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, b_bounds_in_c1));
  aggregated_hit_test_region_list[3].child_count = 0;

  // c2
  aggregated_hit_test_region_list[4].frame_sink_id = c2_id;
  aggregated_hit_test_region_list[4].flags =
      mojom::kHitTestChildSurface | mojom::kHitTestIgnore;
  aggregated_hit_test_region_list[4].transform = transform_e_to_c2;
  aggregated_hit_test_region_list[4].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, c2_bounds_in_e));
  aggregated_hit_test_region_list[4].child_count = 2;

  // g
  aggregated_hit_test_region_list[5].frame_sink_id = g_id;
  aggregated_hit_test_region_list[5].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[5].transform = transform_c2_to_g;
  aggregated_hit_test_region_list[5].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, g_bounds_in_c2));
  aggregated_hit_test_region_list[5].child_count = 0;

  // h
  aggregated_hit_test_region_list[6].frame_sink_id = h_id;
  aggregated_hit_test_region_list[6].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[6].transform = transform_c2_to_h;
  aggregated_hit_test_region_list[6].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, h_bounds_in_c2));
  aggregated_hit_test_region_list[6].child_count = 0;

  // All points are in e's coordinate system when we reach this case.
  gfx::Point point1(1, 1);
  gfx::Point point2(202, 102);
  gfx::Point point3(403, 103);
  gfx::Point point4(202, 202);
  gfx::Point point5(250, 750);
  gfx::Point point6(450, 750);
  gfx::Point point7(350, 1100);

  Target target1 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point1);
  EXPECT_EQ(target1.frame_sink_id, e_id);
  EXPECT_EQ(target1.location_in_target, point1);
  EXPECT_EQ(target1.flags, mojom::kHitTestMine);

  Target target2 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point2);
  EXPECT_EQ(target2.frame_sink_id, a_id);
  EXPECT_EQ(target2.location_in_target, gfx::Point(2, 2));
  EXPECT_EQ(target2.flags, mojom::kHitTestMine);

  Target target3 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point3);
  EXPECT_EQ(target3.frame_sink_id, e_id);
  EXPECT_EQ(target3.location_in_target, point3);
  EXPECT_EQ(target3.flags, mojom::kHitTestMine);

  Target target4 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point4);
  EXPECT_EQ(target4.frame_sink_id, b_id);
  EXPECT_EQ(target4.location_in_target, gfx::Point(2, 2));
  EXPECT_EQ(target4.flags, mojom::kHitTestMine);

  Target target5 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point5);
  EXPECT_EQ(target5.frame_sink_id, g_id);
  EXPECT_EQ(target5.location_in_target, gfx::Point(50, 50));
  EXPECT_EQ(target5.flags, mojom::kHitTestMine);

  Target target6 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point6);
  EXPECT_EQ(target6.frame_sink_id, e_id);
  EXPECT_EQ(target6.location_in_target, point6);
  EXPECT_EQ(target6.flags, mojom::kHitTestMine);

  Target target7 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point7);
  EXPECT_EQ(target7.frame_sink_id, h_id);
  EXPECT_EQ(target7.location_in_target, gfx::Point(150, 300));
  EXPECT_EQ(target7.flags, mojom::kHitTestMine);
}

// Tests transforming location to be in target's coordinate system given the
// target's ancestor list, in the case of
// ClippedChildrenWithTabAndTransparentBackground test.
TEST_F(HitTestQueryTest,
       ClippedChildrenWithTabAndTransparentBackgroundTransform) {
  FrameSinkId e_id = FrameSinkId(1, 1);
  FrameSinkId c1_id = FrameSinkId(2, 2);
  FrameSinkId a_id = FrameSinkId(3, 3);
  FrameSinkId b_id = FrameSinkId(4, 4);
  FrameSinkId c2_id = FrameSinkId(5, 5);
  FrameSinkId g_id = FrameSinkId(6, 6);
  FrameSinkId h_id = FrameSinkId(7, 7);
  gfx::Rect e_bounds_in_e = gfx::Rect(0, 0, 600, 1200);
  gfx::Rect c1_bounds_in_e = gfx::Rect(0, 0, 800, 500);
  gfx::Rect a_bounds_in_c1 = gfx::Rect(0, 0, 200, 100);
  gfx::Rect b_bounds_in_c1 = gfx::Rect(0, 0, 800, 400);
  gfx::Rect c2_bounds_in_e = gfx::Rect(0, 0, 800, 500);
  gfx::Rect g_bounds_in_c2 = gfx::Rect(0, 0, 200, 100);
  gfx::Rect h_bounds_in_c2 = gfx::Rect(0, 0, 800, 800);
  gfx::Transform transform_e_to_e, transform_e_to_c1, transform_c1_to_a,
      transform_c1_to_b, transform_e_to_c2, transform_c2_to_g,
      transform_c2_to_h;
  transform_e_to_c1.Translate(-200, -100);
  transform_c1_to_b.Translate(0, -100);
  transform_e_to_c2.Translate(-200, -700);
  transform_c2_to_h.Translate(0, -100);
  AggregatedHitTestRegion* aggregated_hit_test_region_list =
      aggregated_hit_test_region();

  // e
  aggregated_hit_test_region_list[0].frame_sink_id = e_id;
  aggregated_hit_test_region_list[0].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[0].transform = transform_e_to_e;
  aggregated_hit_test_region_list[0].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, e_bounds_in_e));
  aggregated_hit_test_region_list[0].child_count = 6;

  // c1
  aggregated_hit_test_region_list[1].frame_sink_id = c1_id;
  aggregated_hit_test_region_list[1].flags =
      mojom::kHitTestChildSurface | mojom::kHitTestIgnore;
  aggregated_hit_test_region_list[1].transform = transform_e_to_c1;
  aggregated_hit_test_region_list[1].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, c1_bounds_in_e));
  aggregated_hit_test_region_list[1].child_count = 2;

  // a
  aggregated_hit_test_region_list[2].frame_sink_id = a_id;
  aggregated_hit_test_region_list[2].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[2].transform = transform_c1_to_a;
  aggregated_hit_test_region_list[2].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, a_bounds_in_c1));
  aggregated_hit_test_region_list[2].child_count = 0;

  // b
  aggregated_hit_test_region_list[3].frame_sink_id = b_id;
  aggregated_hit_test_region_list[3].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[3].transform = transform_c1_to_b;
  aggregated_hit_test_region_list[3].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, b_bounds_in_c1));
  aggregated_hit_test_region_list[3].child_count = 0;

  // c2
  aggregated_hit_test_region_list[4].frame_sink_id = c2_id;
  aggregated_hit_test_region_list[4].flags =
      mojom::kHitTestChildSurface | mojom::kHitTestIgnore;
  aggregated_hit_test_region_list[4].transform = transform_e_to_c2;
  aggregated_hit_test_region_list[4].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, c2_bounds_in_e));
  aggregated_hit_test_region_list[4].child_count = 2;

  // g
  aggregated_hit_test_region_list[5].frame_sink_id = g_id;
  aggregated_hit_test_region_list[5].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[5].transform = transform_c2_to_g;
  aggregated_hit_test_region_list[5].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, g_bounds_in_c2));
  aggregated_hit_test_region_list[5].child_count = 0;

  // h
  aggregated_hit_test_region_list[6].frame_sink_id = h_id;
  aggregated_hit_test_region_list[6].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[6].transform = transform_c2_to_h;
  aggregated_hit_test_region_list[6].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, h_bounds_in_c2));
  aggregated_hit_test_region_list[6].child_count = 0;

  // All points are in e's coordinate system when we reach this case.
  gfx::Point point1(1, 1);
  gfx::Point point2(202, 102);
  gfx::Point point3(403, 103);
  gfx::Point point4(202, 202);
  gfx::Point point5(250, 750);
  gfx::Point point6(450, 750);
  gfx::Point point7(350, 1100);

  std::vector<FrameSinkId> target_ancestors1{e_id};
  EXPECT_EQ(hit_test_query().TransformLocationForTarget(
                EventSource::MOUSE, target_ancestors1, point1),
            point1);
  std::vector<FrameSinkId> target_ancestors2{a_id, c1_id, e_id};
  EXPECT_EQ(hit_test_query().TransformLocationForTarget(
                EventSource::MOUSE, target_ancestors2, point2),
            gfx::Point(2, 2));
  EXPECT_EQ(hit_test_query().TransformLocationForTarget(
                EventSource::MOUSE, target_ancestors1, point3),
            point3);
  std::vector<FrameSinkId> target_ancestors3{b_id, c1_id, e_id};
  EXPECT_EQ(hit_test_query().TransformLocationForTarget(
                EventSource::MOUSE, target_ancestors3, point4),
            gfx::Point(2, 2));
  std::vector<FrameSinkId> target_ancestors4{g_id, c2_id, e_id};
  EXPECT_EQ(hit_test_query().TransformLocationForTarget(
                EventSource::MOUSE, target_ancestors4, point5),
            gfx::Point(50, 50));
  EXPECT_EQ(hit_test_query().TransformLocationForTarget(
                EventSource::MOUSE, target_ancestors1, point6),
            point6);
  std::vector<FrameSinkId> target_ancestors5{h_id, c2_id, e_id};
  EXPECT_EQ(hit_test_query().TransformLocationForTarget(
                EventSource::MOUSE, target_ancestors5, point7),
            gfx::Point(150, 300));
}

// Children that are multiple layers deep.
//
//  +e--------------------+
//  |          +c2--------|     Point   maps to
//  | +c1------|----+     |     -----   -------
//  |1| +a-----|---+|     |       1        e
//  | | |+b----|--+||     |       2        g
//  | | ||+g--+|  |||     |       3        b
//  | | ||| 2 || 3|||  4  |       4        c2
//  | | ||+---+|  |||     |
//  | | |+-----|--+||     |
//  | | +------| --+|     |
//  | +--------|----+     |
//  +---------------------+
//
TEST_F(HitTestQueryTest, MultipleLayerChild) {
  FrameSinkId e_id = FrameSinkId(1, 1);
  FrameSinkId c1_id = FrameSinkId(2, 2);
  FrameSinkId a_id = FrameSinkId(3, 3);
  FrameSinkId b_id = FrameSinkId(4, 4);
  FrameSinkId g_id = FrameSinkId(5, 5);
  FrameSinkId c2_id = FrameSinkId(6, 6);
  gfx::Rect e_bounds_in_e = gfx::Rect(0, 0, 1000, 1000);
  gfx::Rect c1_bounds_in_e = gfx::Rect(0, 0, 800, 800);
  gfx::Rect a_bounds_in_c1 = gfx::Rect(0, 0, 700, 700);
  gfx::Rect b_bounds_in_a = gfx::Rect(0, 0, 600, 600);
  gfx::Rect g_bounds_in_b = gfx::Rect(0, 0, 200, 200);
  gfx::Rect c2_bounds_in_e = gfx::Rect(0, 0, 800, 800);
  gfx::Transform transform_e_to_e, transform_e_to_c1, transform_c1_to_a,
      transform_a_to_b, transform_b_to_g, transform_e_to_c2;
  transform_e_to_c1.Translate(-100, -100);
  transform_a_to_b.Translate(-50, -30);
  transform_b_to_g.Translate(-150, -200);
  transform_e_to_c2.Translate(-400, -50);
  AggregatedHitTestRegion* aggregated_hit_test_region_list =
      aggregated_hit_test_region();

  // e
  aggregated_hit_test_region_list[0].frame_sink_id = e_id;
  aggregated_hit_test_region_list[0].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[0].transform = transform_e_to_e;
  aggregated_hit_test_region_list[0].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, e_bounds_in_e));
  aggregated_hit_test_region_list[0].child_count = 5;

  // c1
  aggregated_hit_test_region_list[1].frame_sink_id = c1_id;
  aggregated_hit_test_region_list[1].flags =
      mojom::kHitTestChildSurface | mojom::kHitTestIgnore;
  aggregated_hit_test_region_list[1].transform = transform_e_to_c1;
  aggregated_hit_test_region_list[1].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, c1_bounds_in_e));
  aggregated_hit_test_region_list[1].child_count = 3;

  // a
  aggregated_hit_test_region_list[2].frame_sink_id = a_id;
  aggregated_hit_test_region_list[2].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[2].transform = transform_c1_to_a;
  aggregated_hit_test_region_list[2].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, a_bounds_in_c1));
  aggregated_hit_test_region_list[2].child_count = 2;

  // b
  aggregated_hit_test_region_list[3].frame_sink_id = b_id;
  aggregated_hit_test_region_list[3].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[3].transform = transform_a_to_b;
  aggregated_hit_test_region_list[3].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, b_bounds_in_a));
  aggregated_hit_test_region_list[3].child_count = 1;

  // g
  aggregated_hit_test_region_list[4].frame_sink_id = g_id;
  aggregated_hit_test_region_list[4].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[4].transform = transform_b_to_g;
  aggregated_hit_test_region_list[4].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, g_bounds_in_b));
  aggregated_hit_test_region_list[4].child_count = 0;

  // c2
  aggregated_hit_test_region_list[5].frame_sink_id = c2_id;
  aggregated_hit_test_region_list[5].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[5].transform = transform_e_to_c2;
  aggregated_hit_test_region_list[5].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, c2_bounds_in_e));
  aggregated_hit_test_region_list[5].child_count = 0;

  // All points are in e's coordinate system when we reach this case.
  gfx::Point point1(1, 1);
  gfx::Point point2(300, 350);
  gfx::Point point3(550, 350);
  gfx::Point point4(900, 350);

  Target target1 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point1);
  EXPECT_EQ(target1.frame_sink_id, e_id);
  EXPECT_EQ(target1.location_in_target, point1);
  EXPECT_EQ(target1.flags, mojom::kHitTestMine);

  Target target2 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point2);
  EXPECT_EQ(target2.frame_sink_id, g_id);
  EXPECT_EQ(target2.location_in_target, gfx::Point(0, 20));
  EXPECT_EQ(target2.flags, mojom::kHitTestMine);

  Target target3 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point3);
  EXPECT_EQ(target3.frame_sink_id, b_id);
  EXPECT_EQ(target3.location_in_target, gfx::Point(400, 220));
  EXPECT_EQ(target3.flags, mojom::kHitTestMine);

  Target target4 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point4);
  EXPECT_EQ(target4.frame_sink_id, c2_id);
  EXPECT_EQ(target4.location_in_target, gfx::Point(500, 300));
  EXPECT_EQ(target4.flags, mojom::kHitTestMine);
}

// Multiple layers deep of transparent children.
//
//  +e--------------------+
//  |          +c2--------|     Point   maps to
//  | +c1------|----+     |     -----   -------
//  |1| +a-----|---+|     |       1        e
//  | | |+b----|--+||     |       2        e
//  | | ||+g--+|  |||     |       3        c2
//  | | ||| 2 || 3|||  4  |       4        c2
//  | | ||+---+|  |||     |
//  | | |+-----|--+||     |
//  | | +------| --+|     |
//  | +--------|----+     |
//  +---------------------+
//
TEST_F(HitTestQueryTest, MultipleLayerTransparentChild) {
  FrameSinkId e_id = FrameSinkId(1, 1);
  FrameSinkId c1_id = FrameSinkId(2, 2);
  FrameSinkId a_id = FrameSinkId(3, 3);
  FrameSinkId b_id = FrameSinkId(4, 4);
  FrameSinkId g_id = FrameSinkId(5, 5);
  FrameSinkId c2_id = FrameSinkId(6, 6);
  gfx::Rect e_bounds_in_e = gfx::Rect(0, 0, 1000, 1000);
  gfx::Rect c1_bounds_in_e = gfx::Rect(0, 0, 800, 800);
  gfx::Rect a_bounds_in_c1 = gfx::Rect(0, 0, 700, 700);
  gfx::Rect b_bounds_in_a = gfx::Rect(0, 0, 600, 600);
  gfx::Rect g_bounds_in_b = gfx::Rect(0, 0, 200, 200);
  gfx::Rect c2_bounds_in_e = gfx::Rect(0, 0, 800, 800);
  gfx::Transform transform_e_to_e, transform_e_to_c1, transform_c1_to_a,
      transform_a_to_b, transform_b_to_g, transform_e_to_c2;
  transform_e_to_c1.Translate(-100, -100);
  transform_a_to_b.Translate(-50, -30);
  transform_b_to_g.Translate(-150, -200);
  transform_e_to_c2.Translate(-400, -50);
  AggregatedHitTestRegion* aggregated_hit_test_region_list =
      aggregated_hit_test_region();

  // e
  aggregated_hit_test_region_list[0].frame_sink_id = e_id;
  aggregated_hit_test_region_list[0].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[0].transform = transform_e_to_e;
  aggregated_hit_test_region_list[0].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, e_bounds_in_e));
  aggregated_hit_test_region_list[0].child_count = 5;

  // c1
  aggregated_hit_test_region_list[1].frame_sink_id = c1_id;
  aggregated_hit_test_region_list[1].flags =
      mojom::kHitTestChildSurface | mojom::kHitTestIgnore;
  aggregated_hit_test_region_list[1].transform = transform_e_to_c1;
  aggregated_hit_test_region_list[1].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, c1_bounds_in_e));
  aggregated_hit_test_region_list[1].child_count = 3;

  // a
  aggregated_hit_test_region_list[2].frame_sink_id = a_id;
  aggregated_hit_test_region_list[2].flags =
      mojom::kHitTestChildSurface | mojom::kHitTestIgnore;
  aggregated_hit_test_region_list[2].transform = transform_c1_to_a;
  aggregated_hit_test_region_list[2].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, a_bounds_in_c1));
  aggregated_hit_test_region_list[2].child_count = 2;

  // b
  aggregated_hit_test_region_list[3].frame_sink_id = b_id;
  aggregated_hit_test_region_list[3].flags =
      mojom::kHitTestChildSurface | mojom::kHitTestIgnore;
  aggregated_hit_test_region_list[3].transform = transform_a_to_b;
  aggregated_hit_test_region_list[3].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, b_bounds_in_a));
  aggregated_hit_test_region_list[3].child_count = 1;

  // g
  aggregated_hit_test_region_list[4].frame_sink_id = g_id;
  aggregated_hit_test_region_list[4].flags =
      mojom::kHitTestChildSurface | mojom::kHitTestIgnore;
  aggregated_hit_test_region_list[4].transform = transform_b_to_g;
  aggregated_hit_test_region_list[4].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, g_bounds_in_b));
  aggregated_hit_test_region_list[4].child_count = 0;

  // c2
  aggregated_hit_test_region_list[5].frame_sink_id = c2_id;
  aggregated_hit_test_region_list[5].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[5].transform = transform_e_to_c2;
  aggregated_hit_test_region_list[5].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, c2_bounds_in_e));
  aggregated_hit_test_region_list[5].child_count = 0;

  // All points are in e's coordinate system when we reach this case.
  gfx::Point point1(1, 1);
  gfx::Point point2(300, 350);
  gfx::Point point3(450, 350);
  gfx::Point point4(900, 350);

  Target target1 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point1);
  EXPECT_EQ(target1.frame_sink_id, e_id);
  EXPECT_EQ(target1.location_in_target, point1);
  EXPECT_TRUE(target1.flags);

  Target target2 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point2);
  EXPECT_EQ(target2.frame_sink_id, e_id);
  EXPECT_EQ(target2.location_in_target, point2);
  EXPECT_TRUE(target2.flags);

  Target target3 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point3);
  EXPECT_EQ(target3.frame_sink_id, c2_id);
  EXPECT_EQ(target3.location_in_target, gfx::Point(50, 300));
  EXPECT_TRUE(target3.flags);

  Target target4 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point4);
  EXPECT_EQ(target4.frame_sink_id, c2_id);
  EXPECT_EQ(target4.location_in_target, gfx::Point(500, 300));
  EXPECT_TRUE(target4.flags);
}

TEST_F(HitTestQueryTest, InvalidAggregatedHitTestRegionData) {
  FrameSinkId e_id = FrameSinkId(1, 1);
  FrameSinkId c_id = FrameSinkId(2, 2);
  FrameSinkId a_id = FrameSinkId(3, 3);
  FrameSinkId b_id = FrameSinkId(4, 4);
  gfx::Rect e_bounds_in_e = gfx::Rect(0, 0, 600, 600);
  gfx::Rect c_bounds_in_e = gfx::Rect(0, 0, 800, 800);
  gfx::Rect a_bounds_in_c = gfx::Rect(0, 0, 200, 100);
  gfx::Rect b_bounds_in_c = gfx::Rect(0, 0, 800, 600);
  gfx::Transform transform_e_to_e, transform_e_to_c, transform_c_to_a,
      transform_c_to_b;
  transform_e_to_c.Translate(-200, -100);
  transform_c_to_b.Translate(0, -100);

  // All points are in e's coordinate system when we reach this case.
  gfx::Point point1(1, 1);
  gfx::Point point2(202, 102);

  {
    AggregatedHitTestRegion* aggregated_hit_test_region_list_min =
        aggregated_hit_test_region();

    // e
    aggregated_hit_test_region_list_min[0].frame_sink_id = e_id;
    aggregated_hit_test_region_list_min[0].flags = mojom::kHitTestMine;
    aggregated_hit_test_region_list_min[0].transform = transform_e_to_e;
    aggregated_hit_test_region_list_min[0].rects.push_back(
        HitTestRect(mojom::kHitTestMouse, e_bounds_in_e));
    aggregated_hit_test_region_list_min[0].child_count = 3;

    // c
    aggregated_hit_test_region_list_min[1].frame_sink_id = c_id;
    aggregated_hit_test_region_list_min[1].flags =
        mojom::kHitTestChildSurface | mojom::kHitTestIgnore;
    aggregated_hit_test_region_list_min[1].transform = transform_e_to_c;
    aggregated_hit_test_region_list_min[1].rects.push_back(
        HitTestRect(mojom::kHitTestMouse, c_bounds_in_e));
    aggregated_hit_test_region_list_min[1].child_count = INT32_MIN;

    // a
    aggregated_hit_test_region_list_min[2].frame_sink_id = a_id;
    aggregated_hit_test_region_list_min[2].flags = mojom::kHitTestMine;
    aggregated_hit_test_region_list_min[2].transform = transform_c_to_a;
    aggregated_hit_test_region_list_min[2].rects.push_back(
        HitTestRect(mojom::kHitTestMouse, a_bounds_in_c));
    aggregated_hit_test_region_list_min[2].child_count = 0;

    // b
    aggregated_hit_test_region_list_min[3].frame_sink_id = b_id;
    aggregated_hit_test_region_list_min[3].flags = mojom::kHitTestMine;
    aggregated_hit_test_region_list_min[3].transform = transform_c_to_b;
    aggregated_hit_test_region_list_min[3].rects.push_back(
        HitTestRect(mojom::kHitTestMouse, b_bounds_in_c));
    aggregated_hit_test_region_list_min[3].child_count = 0;

    // |child_count| is invalid, which is a security fault. For now, check to
    // see if the returned Target is invalid.
    Target target1 =
        hit_test_query().FindTargetForLocation(EventSource::MOUSE, point1);
    EXPECT_EQ(target1.frame_sink_id, FrameSinkId());
    EXPECT_EQ(target1.location_in_target, gfx::Point());
    EXPECT_FALSE(target1.flags);

    Target target2 =
        hit_test_query().FindTargetForLocation(EventSource::MOUSE, point2);
    EXPECT_EQ(target2.frame_sink_id, FrameSinkId());
    EXPECT_EQ(target2.location_in_target, gfx::Point());
    EXPECT_FALSE(target2.flags);
  }

  {
    AggregatedHitTestRegion* aggregated_hit_test_region_list_max =
        aggregated_hit_test_region();

    // e
    aggregated_hit_test_region_list_max[0].frame_sink_id = e_id;
    aggregated_hit_test_region_list_max[0].flags = mojom::kHitTestMine;
    aggregated_hit_test_region_list_max[0].transform = transform_e_to_e;
    aggregated_hit_test_region_list_max[0].rects.push_back(
        HitTestRect(mojom::kHitTestMouse, e_bounds_in_e));
    aggregated_hit_test_region_list_max[0].child_count = INT32_MAX;

    // c
    aggregated_hit_test_region_list_max[1].frame_sink_id = c_id;
    aggregated_hit_test_region_list_max[1].flags =
        mojom::kHitTestChildSurface | mojom::kHitTestIgnore;
    aggregated_hit_test_region_list_max[1].transform = transform_e_to_c;
    aggregated_hit_test_region_list_max[1].rects.push_back(
        HitTestRect(mojom::kHitTestMouse, c_bounds_in_e));
    aggregated_hit_test_region_list_max[1].child_count = 2;

    // a
    aggregated_hit_test_region_list_max[2].frame_sink_id = a_id;
    aggregated_hit_test_region_list_max[2].flags = mojom::kHitTestMine;
    aggregated_hit_test_region_list_max[2].transform = transform_c_to_a;
    aggregated_hit_test_region_list_max[2].rects.push_back(
        HitTestRect(mojom::kHitTestMouse, a_bounds_in_c));
    aggregated_hit_test_region_list_max[2].child_count = 0;

    // b
    aggregated_hit_test_region_list_max[3].frame_sink_id = b_id;
    aggregated_hit_test_region_list_max[3].flags = mojom::kHitTestMine;
    aggregated_hit_test_region_list_max[3].transform = transform_c_to_b;
    aggregated_hit_test_region_list_max[3].rects.push_back(
        HitTestRect(mojom::kHitTestMouse, b_bounds_in_c));
    aggregated_hit_test_region_list_max[3].child_count = 0;

    Target target3 =
        hit_test_query().FindTargetForLocation(EventSource::MOUSE, point1);
    EXPECT_EQ(target3.frame_sink_id, FrameSinkId());
    EXPECT_EQ(target3.location_in_target, gfx::Point());
    EXPECT_FALSE(target3.flags);
  }

  {
    AggregatedHitTestRegion* aggregated_hit_test_region_list_bigger =
        aggregated_hit_test_region();

    // e
    aggregated_hit_test_region_list_bigger[0].frame_sink_id = e_id;
    aggregated_hit_test_region_list_bigger[0].flags = mojom::kHitTestMine;
    aggregated_hit_test_region_list_bigger[0].transform = transform_e_to_e;
    aggregated_hit_test_region_list_bigger[0].rects.push_back(
        HitTestRect(mojom::kHitTestMouse, e_bounds_in_e));
    aggregated_hit_test_region_list_bigger[0].child_count = 3;

    // c
    aggregated_hit_test_region_list_bigger[1].frame_sink_id = c_id;
    aggregated_hit_test_region_list_bigger[1].flags =
        mojom::kHitTestChildSurface | mojom::kHitTestIgnore;
    aggregated_hit_test_region_list_bigger[1].transform = transform_e_to_c;
    aggregated_hit_test_region_list_bigger[1].rects.push_back(
        HitTestRect(mojom::kHitTestMouse, c_bounds_in_e));
    aggregated_hit_test_region_list_bigger[1].child_count = 3;

    // a
    aggregated_hit_test_region_list_bigger[2].frame_sink_id = a_id;
    aggregated_hit_test_region_list_bigger[2].flags = mojom::kHitTestMine;
    aggregated_hit_test_region_list_bigger[2].transform = transform_c_to_a;
    aggregated_hit_test_region_list_bigger[2].rects.push_back(
        HitTestRect(mojom::kHitTestMouse, a_bounds_in_c));
    aggregated_hit_test_region_list_bigger[2].child_count = 0;

    // b
    aggregated_hit_test_region_list_bigger[3].frame_sink_id = b_id;
    aggregated_hit_test_region_list_bigger[3].flags = mojom::kHitTestMine;
    aggregated_hit_test_region_list_bigger[3].transform = transform_c_to_b;
    aggregated_hit_test_region_list_bigger[3].rects.push_back(
        HitTestRect(mojom::kHitTestMouse, b_bounds_in_c));
    aggregated_hit_test_region_list_bigger[3].child_count = 0;

    Target target4 =
        hit_test_query().FindTargetForLocation(EventSource::MOUSE, point1);
    EXPECT_EQ(target4.frame_sink_id, FrameSinkId());
    EXPECT_EQ(target4.location_in_target, gfx::Point());
    EXPECT_FALSE(target4.flags);
  }
}

// Tests flags kHitTestMouse and kHitTestTouch.
TEST_F(HitTestQueryTest, MouseTouchFlags) {
  FrameSinkId e_id = FrameSinkId(1, 1);
  FrameSinkId c1_id = FrameSinkId(2, 2);
  FrameSinkId c2_id = FrameSinkId(3, 3);
  gfx::Rect e_bounds_in_e = gfx::Rect(0, 0, 600, 600);
  gfx::Rect c1_bounds_in_e = gfx::Rect(0, 0, 300, 200);
  gfx::Rect c2_bounds_in_e = gfx::Rect(0, 0, 350, 250);
  gfx::Transform transform_e_to_e, transform_e_to_c1, transform_e_to_c2;
  transform_e_to_c1.Translate(-100, -100);
  transform_e_to_c2.Translate(-75, -75);
  AggregatedHitTestRegion* aggregated_hit_test_region_list =
      aggregated_hit_test_region();

  // e
  aggregated_hit_test_region_list[0].frame_sink_id = e_id;
  aggregated_hit_test_region_list[0].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[0].transform = transform_e_to_e;
  aggregated_hit_test_region_list[0].rects.push_back(
      HitTestRect(mojom::kHitTestMouse | mojom::kHitTestTouch, e_bounds_in_e));
  aggregated_hit_test_region_list[0].child_count = 2;

  // c1
  aggregated_hit_test_region_list[1].frame_sink_id = c1_id;
  aggregated_hit_test_region_list[1].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[1].transform = transform_e_to_c1;
  aggregated_hit_test_region_list[1].rects.push_back(
      HitTestRect(mojom::kHitTestMouse, c1_bounds_in_e));
  aggregated_hit_test_region_list[1].child_count = 0;

  // c2
  aggregated_hit_test_region_list[2].frame_sink_id = c2_id;
  aggregated_hit_test_region_list[2].flags = mojom::kHitTestMine;
  aggregated_hit_test_region_list[2].transform = transform_e_to_c2;
  aggregated_hit_test_region_list[2].rects.push_back(
      HitTestRect(mojom::kHitTestTouch, c2_bounds_in_e));
  aggregated_hit_test_region_list[2].rects.push_back(
      HitTestRect(mojom::kHitTestTouch, gfx::Rect(500, 500, 20, 20)));
  aggregated_hit_test_region_list[2].child_count = 0;

  // All points are in e's coordinate system when we reach this case.
  gfx::Point point1(80, 80);
  gfx::Point point2(150, 150);
  gfx::Point point3(580, 580);

  Target target1 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point1);
  EXPECT_EQ(target1.frame_sink_id, e_id);
  EXPECT_EQ(target1.location_in_target, point1);
  EXPECT_EQ(target1.flags, mojom::kHitTestMine);

  Target target2 =
      hit_test_query().FindTargetForLocation(EventSource::TOUCH, point1);
  EXPECT_EQ(target2.frame_sink_id, c2_id);
  EXPECT_EQ(target2.location_in_target, gfx::Point(5, 5));
  EXPECT_EQ(target2.flags, mojom::kHitTestMine);

  Target target3 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point2);
  EXPECT_EQ(target3.frame_sink_id, c1_id);
  EXPECT_EQ(target3.location_in_target, gfx::Point(50, 50));
  EXPECT_EQ(target3.flags, mojom::kHitTestMine);

  Target target4 =
      hit_test_query().FindTargetForLocation(EventSource::TOUCH, point2);
  EXPECT_EQ(target4.frame_sink_id, c2_id);
  EXPECT_EQ(target4.location_in_target, gfx::Point(75, 75));
  EXPECT_EQ(target4.flags, mojom::kHitTestMine);

  Target target5 =
      hit_test_query().FindTargetForLocation(EventSource::MOUSE, point3);
  EXPECT_EQ(target5.frame_sink_id, e_id);
  EXPECT_EQ(target5.location_in_target, point3);
  EXPECT_EQ(target1.flags, mojom::kHitTestMine);

  Target target6 =
      hit_test_query().FindTargetForLocation(EventSource::TOUCH, point3);
  EXPECT_EQ(target6.frame_sink_id, c2_id);
  EXPECT_EQ(target6.location_in_target, gfx::Point(5, 5));
  EXPECT_EQ(target6.flags, mojom::kHitTestMine);
}

}  // namespace test
}  // namespace viz
