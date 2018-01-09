// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/repositioner.h"

#include "chrome/browser/vr/pose_util.h"
#include "chrome/browser/vr/ui_scene_constants.h"
#include "ui/gfx/geometry/angle_conversions.h"
#include "ui/gfx/geometry/quaternion.h"

namespace vr {

Repositioner::Repositioner() {
  set_hit_testable(false);
}

Repositioner::~Repositioner() = default;

gfx::Transform Repositioner::LocalTransform() const {
  return transform_;
}

gfx::Transform Repositioner::GetTargetLocalTransform() const {
  return transform_;
}

void Repositioner::UpdateTransform(const gfx::Transform& head_pose) {
  if (!dragging_) {
    gfx::Point3F initial_reticle_point = target_point_;
    transform_.TransformPointReverse(&initial_reticle_point);
    initial_reticle_vector_ = initial_reticle_point - kOrigin;
    dragging_ = true;
  }

  gfx::Vector3dF hit_test_origin_vector = hit_test_origin_ - kOrigin;
  gfx::Vector3dF normalized_hit_test_direction;
  bool success =
      hit_test_direction_.GetNormalized(&normalized_hit_test_direction);
  DCHECK(success);
  float d1 =
      -gfx::DotProduct(hit_test_origin_vector, normalized_hit_test_direction);
  float d2 = std::sqrt(initial_reticle_vector_.LengthSquared() -
                       (hit_test_origin_vector.LengthSquared() - d1 * d1));
  float new_ray_distance = d1 + d2;
  gfx::Vector3dF new_ray =
      gfx::ScaleVector3d(normalized_hit_test_direction, new_ray_distance);
  gfx::Vector3dF new_reticle_vector = hit_test_origin_vector;
  new_reticle_vector.Add(new_ray);

  // Phase one: rotate the world so that reticle is at the same place after
  // rotation.
  gfx::Quaternion rotate_to_new_position(initial_reticle_vector_,
                                         new_reticle_vector);
  gfx::Transform phase_one_transform = gfx::Transform(rotate_to_new_position);
  gfx::Vector3dF intermediate_right_vector = {1, 0, 0};
  phase_one_transform.TransformVector(&intermediate_right_vector);

  // Phase two: rotate right vector to expected right vector along
  // new_retice_vector.
  gfx::Vector3dF head_up_vector = vr::GetUpVector(head_pose);
  float dot = gfx::DotProduct(new_reticle_vector, head_up_vector);
  // ref_vector is the projected vector of new_reticle_vector on the plane
  // defined by kOrigin (point) and head_up_vector (normal).
  gfx::Vector3dF ref_vector =
      new_reticle_vector + gfx::ScaleVector3d(head_up_vector, -dot);
  float min_angle_to_plane =
      gfx::AngleBetweenVectorsInDegrees(new_reticle_vector, ref_vector);
  float angle = gfx::AngleBetweenVectorsInDegrees(new_reticle_vector,
                                                  intermediate_right_vector);

  if (angle < min_angle_to_plane || angle > 180.f - min_angle_to_plane)
    return;

  // Rotate ref_vector to expected right vector along new_retice_vector.
  float theta = std::acos(std::cos(gfx::DegToRad(angle)) /
                          std::cos(gfx::DegToRad(min_angle_to_plane)));
  gfx::Quaternion rotate(head_up_vector, -theta);
  gfx::Transform(rotate).TransformVector(&ref_vector);

  gfx::Vector3dF normalized_expect_right;
  ref_vector.GetNormalized(&normalized_expect_right);

  gfx::Vector3dF normalized_new_reticle;
  new_reticle_vector.GetNormalized(&normalized_new_reticle);

  gfx::Vector3dF rotation_center = gfx::ScaleVector3d(
      normalized_new_reticle,
      gfx::DotProduct(normalized_new_reticle, intermediate_right_vector));
  gfx::Vector3dF vec_i = intermediate_right_vector - rotation_center;
  gfx::Vector3dF vec_e = normalized_expect_right - rotation_center;
  gfx::Quaternion rotate_to_expected_right(vec_i, vec_e);

  transform_.MakeIdentity();
  transform_.ConcatTransform(phase_one_transform);
  transform_.ConcatTransform(gfx::Transform(rotate_to_expected_right));
}

bool Repositioner::OnBeginFrame(const base::TimeTicks& time,
                                const gfx::Transform& head_pose) {
  if (enabled_) {
    UpdateTransform(head_pose);
    return true;
  }
  dragging_ = false;
  return false;
}

void Repositioner::DumpGeometry(std::ostringstream* os) const {
  gfx::Transform t = world_space_transform();
  gfx::Vector3dF forward = {0, 0, -1};
  t.TransformVector(&forward);
  // Decompose the rotation to world x axis followed by world y axis
  float x_rotation = std::asin(forward.y() / forward.Length());
  gfx::Vector3dF projected_forward = {forward.x(), 0, forward.z()};
  float y_rotation = std::acos(gfx::DotProduct(projected_forward, {0, 0, -1}) /
                               projected_forward.Length());
  if (projected_forward.x() > 0.f)
    y_rotation *= -1;
  *os << "rx(" << gfx::RadToDeg(x_rotation) << ") "
      << "ry(" << gfx::RadToDeg(y_rotation) << ") ";
}

}  // namespace vr
