// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/markers/UnsortedDocumentMarkerListEditor.h"

#include "core/editing/markers/TextMatchMarker.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class UnsortedDocumentMarkerListEditorTest : public ::testing::Test {
 protected:
  DocumentMarker* CreateMarker(unsigned startOffset, unsigned endOffset) {
    return new TextMatchMarker(startOffset, endOffset,
                               TextMatchMarker::MatchStatus::kInactive);
  }

  PersistentHeapVector<Member<DocumentMarker>> markers_;
};

namespace {

bool compare_markers(const Member<DocumentMarker>& marker1,
                     const Member<DocumentMarker>& marker2) {
  if (marker1->StartOffset() != marker2->StartOffset())
    return marker1->StartOffset() < marker2->StartOffset();

  return marker1->EndOffset() < marker2->EndOffset();
}

}  // namespace

TEST_F(UnsortedDocumentMarkerListEditorTest, MoveMarkers) {
  markers_.push_back(CreateMarker(30, 40));
  markers_.push_back(CreateMarker(0, 30));
  markers_.push_back(CreateMarker(10, 40));
  markers_.push_back(CreateMarker(0, 20));
  markers_.push_back(CreateMarker(0, 40));
  markers_.push_back(CreateMarker(20, 40));
  markers_.push_back(CreateMarker(20, 30));
  markers_.push_back(CreateMarker(0, 10));
  markers_.push_back(CreateMarker(10, 30));
  markers_.push_back(CreateMarker(10, 20));
  markers_.push_back(CreateMarker(11, 21));

  DocumentMarkerList* dst_list = new SuggestionMarkerListImpl();
  // The markers with start and end offset < 11 should be moved to dst_list.
  // Markers that start before 11 and end at 11 or later should be removed.
  // Markers that start at 11 or later should not be moved.
  UnsortedDocumentMarkerListEditor::MoveMarkers(&markers_, 11, dst_list);

  std::sort(markers_.begin(), markers_.end(), compare_markers);

  EXPECT_EQ(4u, markers_.size());

  EXPECT_EQ(11u, markers_[0]->StartOffset());
  EXPECT_EQ(21u, markers_[0]->EndOffset());

  EXPECT_EQ(20u, markers_[1]->StartOffset());
  EXPECT_EQ(30u, markers_[1]->EndOffset());

  EXPECT_EQ(20u, markers_[2]->StartOffset());
  EXPECT_EQ(40u, markers_[2]->EndOffset());

  EXPECT_EQ(30u, markers_[3]->StartOffset());
  EXPECT_EQ(40u, markers_[3]->EndOffset());

  DocumentMarkerVector dst_list_markers = dst_list->GetMarkers();
  std::sort(dst_list_markers.begin(), dst_list_markers.end(), compare_markers);

  // Markers
  EXPECT_EQ(1u, dst_list_markers.size());

  EXPECT_EQ(0u, dst_list_markers[0]->StartOffset());
  EXPECT_EQ(10u, dst_list_markers[0]->EndOffset());
}

TEST_F(UnsortedDocumentMarkerListEditorTest, RemoveMarkersEmptyList) {
  EXPECT_FALSE(
      UnsortedDocumentMarkerListEditor::RemoveMarkers(&markers_, 0, 10));
  EXPECT_EQ(0u, markers_.size());
}

TEST_F(UnsortedDocumentMarkerListEditorTest, RemoveMarkersTouchingEndpoints) {
  markers_.push_back(CreateMarker(30, 40));
  markers_.push_back(CreateMarker(40, 50));
  markers_.push_back(CreateMarker(10, 20));
  markers_.push_back(CreateMarker(0, 10));
  markers_.push_back(CreateMarker(20, 30));

  EXPECT_TRUE(
      UnsortedDocumentMarkerListEditor::RemoveMarkers(&markers_, 20, 10));

  std::sort(markers_.begin(), markers_.end(), compare_markers);

  EXPECT_EQ(4u, markers_.size());

  EXPECT_EQ(0u, markers_[0]->StartOffset());
  EXPECT_EQ(10u, markers_[0]->EndOffset());

  EXPECT_EQ(10u, markers_[1]->StartOffset());
  EXPECT_EQ(20u, markers_[1]->EndOffset());

  EXPECT_EQ(30u, markers_[2]->StartOffset());
  EXPECT_EQ(40u, markers_[2]->EndOffset());

  EXPECT_EQ(40u, markers_[3]->StartOffset());
  EXPECT_EQ(50u, markers_[3]->EndOffset());
}

TEST_F(UnsortedDocumentMarkerListEditorTest,
       RemoveMarkersOneCharacterIntoInterior) {
  markers_.push_back(CreateMarker(30, 40));
  markers_.push_back(CreateMarker(40, 50));
  markers_.push_back(CreateMarker(10, 20));
  markers_.push_back(CreateMarker(0, 10));
  markers_.push_back(CreateMarker(20, 30));

  EXPECT_TRUE(
      UnsortedDocumentMarkerListEditor::RemoveMarkers(&markers_, 19, 12));

  std::sort(markers_.begin(), markers_.end(), compare_markers);

  EXPECT_EQ(2u, markers_.size());

  EXPECT_EQ(0u, markers_[0]->StartOffset());
  EXPECT_EQ(10u, markers_[0]->EndOffset());

  EXPECT_EQ(40u, markers_[1]->StartOffset());
  EXPECT_EQ(50u, markers_[1]->EndOffset());
}

TEST_F(UnsortedDocumentMarkerListEditorTest,
       FirstMarkerIntersectingRange_Empty) {
  DocumentMarker* marker =
      UnsortedDocumentMarkerListEditor::FirstMarkerIntersectingRange(markers_,
                                                                     0, 10);
  EXPECT_EQ(nullptr, marker);
}

TEST_F(UnsortedDocumentMarkerListEditorTest,
       FirstMarkerIntersectingRange_EmptyRange) {
  markers_.push_back(CreateMarker(0, 5));
  DocumentMarker* marker =
      UnsortedDocumentMarkerListEditor::FirstMarkerIntersectingRange(markers_,
                                                                     5, 15);
  EXPECT_EQ(nullptr, marker);
}

TEST_F(UnsortedDocumentMarkerListEditorTest,
       FirstMarkerIntersectingRange_TouchingStart) {
  markers_.push_back(CreateMarker(1, 10));
  markers_.push_back(CreateMarker(0, 10));

  DocumentMarker* marker =
      UnsortedDocumentMarkerListEditor::FirstMarkerIntersectingRange(markers_,
                                                                     0, 1);

  EXPECT_NE(nullptr, marker);
  EXPECT_EQ(0u, marker->StartOffset());
  EXPECT_EQ(10u, marker->EndOffset());
}

TEST_F(UnsortedDocumentMarkerListEditorTest,
       FirstMarkerIntersectingRange_TouchingEnd) {
  markers_.push_back(CreateMarker(0, 9));
  markers_.push_back(CreateMarker(0, 10));

  DocumentMarker* marker =
      UnsortedDocumentMarkerListEditor::FirstMarkerIntersectingRange(markers_,
                                                                     9, 10);

  EXPECT_NE(nullptr, marker);
  EXPECT_EQ(0u, marker->StartOffset());
  EXPECT_EQ(10u, marker->EndOffset());
}

TEST_F(UnsortedDocumentMarkerListEditorTest,
       MarkersIntersectingRange_TouchingStart) {
  markers_.push_back(CreateMarker(0, 9));
  markers_.push_back(CreateMarker(1, 9));
  markers_.push_back(CreateMarker(0, 10));
  markers_.push_back(CreateMarker(1, 10));

  UnsortedDocumentMarkerListEditor::MarkerList markers__intersecting_range =
      UnsortedDocumentMarkerListEditor::MarkersIntersectingRange(markers_, 0,
                                                                 1);

  EXPECT_EQ(2u, markers__intersecting_range.size());

  EXPECT_EQ(0u, markers__intersecting_range[0]->StartOffset());
  EXPECT_EQ(9u, markers__intersecting_range[0]->EndOffset());

  EXPECT_EQ(0u, markers__intersecting_range[1]->StartOffset());
  EXPECT_EQ(10u, markers__intersecting_range[1]->EndOffset());
}

TEST_F(UnsortedDocumentMarkerListEditorTest,
       MarkersIntersectingRange_TouchingEnd) {
  markers_.push_back(CreateMarker(0, 9));
  markers_.push_back(CreateMarker(1, 9));
  markers_.push_back(CreateMarker(0, 10));
  markers_.push_back(CreateMarker(1, 10));

  UnsortedDocumentMarkerListEditor::MarkerList markers_intersecting_range =
      UnsortedDocumentMarkerListEditor::MarkersIntersectingRange(markers_, 9,
                                                                 10);

  EXPECT_EQ(2u, markers_intersecting_range.size());

  EXPECT_EQ(0u, markers_intersecting_range[0]->StartOffset());
  EXPECT_EQ(10u, markers_intersecting_range[0]->EndOffset());

  EXPECT_EQ(1u, markers_intersecting_range[1]->StartOffset());
  EXPECT_EQ(10u, markers_intersecting_range[1]->EndOffset());
}

}  // namespace blink
