// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/surfaces/referenced_surface_tracker.h"

#include <memory>

#include "base/containers/flat_set.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "cc/surfaces/frame_sink_id.h"
#include "cc/surfaces/surface_id.h"
#include "cc/surfaces/surface_reference.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::UnorderedElementsAre;
using testing::IsEmpty;

namespace cc {
namespace test {
namespace {

constexpr FrameSinkId kParentFrameSink(2, 1);
constexpr FrameSinkId kChildFrameSink1(65563, 1);
constexpr FrameSinkId kChildFrameSink2(65564, 1);

const base::flat_set<SurfaceId> kEmptySurfaceIdSet;

base::flat_set<SurfaceId> ReferenceSet(
    std::initializer_list<SurfaceId> surface_ids) {
  return base::flat_set<SurfaceId>(surface_ids, base::KEEP_FIRST_OF_DUPES);
}

SurfaceId MakeSurfaceId(const FrameSinkId& frame_sink_id, uint32_t local_id) {
  return SurfaceId(
      frame_sink_id,
      LocalSurfaceId(local_id, base::UnguessableToken::Deserialize(0, 1u)));
}

}  // namespace

class ReferencedSurfaceTrackerTest : public testing::Test {
 public:
  ReferencedSurfaceTrackerTest() {}
  ~ReferencedSurfaceTrackerTest() override {}

  const std::vector<SurfaceReference>& references_to_remove() {
    return references_to_remove_;
  }

  const std::vector<SurfaceReference>& references_to_add() {
    return references_to_add_;
  }

  void UpdateReferences(
      const SurfaceId& surface_id,
      const base::flat_set<SurfaceId>& old_referenced_surfaces,
      const base::flat_set<SurfaceId>& new_referenced_surfaces) {
    references_to_add_.clear();
    references_to_remove_.clear();
    GetSurfaceReferenceDifference(surface_id, old_referenced_surfaces,
                                  new_referenced_surfaces, &references_to_add_,
                                  &references_to_remove_);
  }

  // testing::Test:
  void SetUp() override {
    references_to_add_.clear();
    references_to_remove_.clear();
  }

 private:
  std::vector<SurfaceReference> references_to_add_;
  std::vector<SurfaceReference> references_to_remove_;

  DISALLOW_COPY_AND_ASSIGN(ReferencedSurfaceTrackerTest);
};

TEST_F(ReferencedSurfaceTrackerTest, RefSurface) {
  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id1 = MakeSurfaceId(kChildFrameSink1, 1);
  const SurfaceReference reference(parent_id, child_id1);

  // Check that reference to |child_id1| is added.
  UpdateReferences(parent_id, kEmptySurfaceIdSet, ReferenceSet({child_id1}));
  EXPECT_THAT(references_to_add(), UnorderedElementsAre(reference));
  EXPECT_THAT(references_to_remove(), IsEmpty());
}

TEST_F(ReferencedSurfaceTrackerTest, NoChangeToReferences) {
  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id1 = MakeSurfaceId(kChildFrameSink1, 1);
  const SurfaceReference reference(parent_id, child_id1);

  // Check that no references are added or removed.
  auto referenced_surfaces = ReferenceSet({child_id1});
  UpdateReferences(parent_id, referenced_surfaces, referenced_surfaces);
  EXPECT_THAT(references_to_remove(), IsEmpty());
  EXPECT_THAT(references_to_add(), IsEmpty());
}

TEST_F(ReferencedSurfaceTrackerTest, UnrefSurface) {
  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id1 = MakeSurfaceId(kChildFrameSink1, 1);
  const SurfaceReference reference(parent_id, child_id1);

  // Check that reference to |child_id1| is removed.
  UpdateReferences(parent_id, ReferenceSet({child_id1}), kEmptySurfaceIdSet);
  EXPECT_THAT(references_to_add(), IsEmpty());
  EXPECT_THAT(references_to_remove(), UnorderedElementsAre(reference));
}

TEST_F(ReferencedSurfaceTrackerTest, RefNewSurfaceForFrameSink) {
  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id1_first = MakeSurfaceId(kChildFrameSink1, 1);
  const SurfaceId child_id1_second = MakeSurfaceId(kChildFrameSink1, 2);
  const SurfaceReference reference_first(parent_id, child_id1_first);
  const SurfaceReference reference_second(parent_id, child_id1_second);

  // Second frame has reference to |child_id1_second| which has the same
  // FrameSinkId but different LocalSurfaceId. Check that first reference is
  // removed and second reference is added.
  UpdateReferences(parent_id, ReferenceSet({child_id1_first}),
                   ReferenceSet({child_id1_second}));
  EXPECT_THAT(references_to_remove(), UnorderedElementsAre(reference_first));
  EXPECT_THAT(references_to_add(), UnorderedElementsAre(reference_second));
}

TEST_F(ReferencedSurfaceTrackerTest, RefTwoThenUnrefOneSurface) {
  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id1 = MakeSurfaceId(kChildFrameSink1, 1);
  const SurfaceId child_id2 = MakeSurfaceId(kChildFrameSink2, 2);
  const SurfaceReference reference1(parent_id, child_id1);
  const SurfaceReference reference2(parent_id, child_id2);

  // First frame references both surfaces.
  const auto initial_referenced = ReferenceSet({child_id1, child_id2});
  UpdateReferences(parent_id, kEmptySurfaceIdSet, initial_referenced);
  EXPECT_THAT(references_to_add(),
              UnorderedElementsAre(reference1, reference2));

  // Second frame references only |child_id2|, check that reference to
  // |child_id1| is removed.
  UpdateReferences(parent_id, initial_referenced, ReferenceSet({child_id2}));
  EXPECT_THAT(references_to_remove(), UnorderedElementsAre(reference1));
  EXPECT_THAT(references_to_add(), IsEmpty());
}

}  // namespace test
}  // namespace cc
