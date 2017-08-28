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
};

namespace {

bool compare_markers(const Member<DocumentMarker>& marker1,
                     const Member<DocumentMarker>& marker2) {
  if (marker1->StartOffset() != marker2->StartOffset())
    return marker1->StartOffset() < marker2->StartOffset();

  return marker1->EndOffset() < marker2->EndOffset();
}

}  // namespace

TEST_F(UnsortedDocumentMarkerListEditorTest, RemoveMarkersEmptyList) {
  UnsortedDocumentMarkerListEditor::MarkerList markers;
  EXPECT_FALSE(
      UnsortedDocumentMarkerListEditor::RemoveMarkers(&markers, 0, 10));
  EXPECT_EQ(0u, markers.size());
}

TEST_F(UnsortedDocumentMarkerListEditorTest, RemoveMarkersTouchingEndpoints) {
  UnsortedDocumentMarkerListEditor::MarkerList markers;
  markers.push_back(CreateMarker(30, 40));
  markers.push_back(CreateMarker(40, 50));
  markers.push_back(CreateMarker(10, 20));
  markers.push_back(CreateMarker(0, 10));
  markers.push_back(CreateMarker(20, 30));

  EXPECT_TRUE(
      UnsortedDocumentMarkerListEditor::RemoveMarkers(&markers, 20, 10));

  std::sort(markers.begin(), markers.end(), compare_markers);

  EXPECT_EQ(4u, markers.size());

  EXPECT_EQ(0u, markers[0]->StartOffset());
  EXPECT_EQ(10u, markers[0]->EndOffset());

  EXPECT_EQ(10u, markers[1]->StartOffset());
  EXPECT_EQ(20u, markers[1]->EndOffset());

  EXPECT_EQ(30u, markers[2]->StartOffset());
  EXPECT_EQ(40u, markers[2]->EndOffset());

  EXPECT_EQ(40u, markers[3]->StartOffset());
  EXPECT_EQ(50u, markers[3]->EndOffset());
}

TEST_F(UnsortedDocumentMarkerListEditorTest,
       RemoveMarkersOneCharacterIntoInterior) {
  UnsortedDocumentMarkerListEditor::MarkerList markers;
  markers.push_back(CreateMarker(30, 40));
  markers.push_back(CreateMarker(40, 50));
  markers.push_back(CreateMarker(10, 20));
  markers.push_back(CreateMarker(0, 10));
  markers.push_back(CreateMarker(20, 30));

  EXPECT_TRUE(
      UnsortedDocumentMarkerListEditor::RemoveMarkers(&markers, 19, 12));

  std::sort(markers.begin(), markers.end(), compare_markers);

  EXPECT_EQ(2u, markers.size());

  EXPECT_EQ(0u, markers[0]->StartOffset());
  EXPECT_EQ(10u, markers[0]->EndOffset());

  EXPECT_EQ(40u, markers[1]->StartOffset());
  EXPECT_EQ(50u, markers[1]->EndOffset());
}

TEST_F(UnsortedDocumentMarkerListEditorTest,
       FirstMarkerIntersectingRange_Empty) {
  UnsortedDocumentMarkerListEditor::MarkerList markers;
  DocumentMarker* marker =
      UnsortedDocumentMarkerListEditor::FirstMarkerIntersectingRange(markers, 0,
                                                                     10);
  EXPECT_EQ(nullptr, marker);
}

TEST_F(UnsortedDocumentMarkerListEditorTest,
       FirstMarkerIntersectingRange_TouchingStart) {
  UnsortedDocumentMarkerListEditor::MarkerList markers;
  markers.push_back(CreateMarker(1, 10));
  markers.push_back(CreateMarker(0, 10));

  DocumentMarker* marker =
      UnsortedDocumentMarkerListEditor::FirstMarkerIntersectingRange(markers, 0,
                                                                     1);

  EXPECT_NE(nullptr, marker);
  EXPECT_EQ(0u, marker->StartOffset());
  EXPECT_EQ(10u, marker->EndOffset());
}

TEST_F(UnsortedDocumentMarkerListEditorTest,
       FirstMarkerIntersectingRange_TouchingEnd) {
  UnsortedDocumentMarkerListEditor::MarkerList markers;
  markers.push_back(CreateMarker(0, 9));
  markers.push_back(CreateMarker(0, 10));

  DocumentMarker* marker =
      UnsortedDocumentMarkerListEditor::FirstMarkerIntersectingRange(markers, 9,
                                                                     10);

  EXPECT_NE(nullptr, marker);
  EXPECT_EQ(0u, marker->StartOffset());
  EXPECT_EQ(10u, marker->EndOffset());
}

TEST_F(UnsortedDocumentMarkerListEditorTest,
       MarkersIntersectingRange_TouchingStart) {
  UnsortedDocumentMarkerListEditor::MarkerList markers;
  markers.push_back(CreateMarker(0, 9));
  markers.push_back(CreateMarker(1, 9));
  markers.push_back(CreateMarker(0, 10));
  markers.push_back(CreateMarker(1, 10));

  UnsortedDocumentMarkerListEditor::MarkerList markers_intersecting_range =
      UnsortedDocumentMarkerListEditor::MarkersIntersectingRange(markers, 0, 1);

  EXPECT_EQ(2u, markers_intersecting_range.size());

  EXPECT_EQ(0u, markers_intersecting_range[0]->StartOffset());
  EXPECT_EQ(9u, markers_intersecting_range[0]->EndOffset());

  EXPECT_EQ(0u, markers_intersecting_range[1]->StartOffset());
  EXPECT_EQ(10u, markers_intersecting_range[1]->EndOffset());
}

TEST_F(UnsortedDocumentMarkerListEditorTest,
       MarkersIntersectingRange_TouchingEnd) {
  UnsortedDocumentMarkerListEditor::MarkerList markers;
  markers.push_back(CreateMarker(0, 9));
  markers.push_back(CreateMarker(1, 9));
  markers.push_back(CreateMarker(0, 10));
  markers.push_back(CreateMarker(1, 10));

  UnsortedDocumentMarkerListEditor::MarkerList markers_intersecting_range =
      UnsortedDocumentMarkerListEditor::MarkersIntersectingRange(markers, 9,
                                                                 10);

  EXPECT_EQ(2u, markers_intersecting_range.size());

  EXPECT_EQ(0u, markers_intersecting_range[0]->StartOffset());
  EXPECT_EQ(10u, markers_intersecting_range[0]->EndOffset());

  EXPECT_EQ(1u, markers_intersecting_range[1]->StartOffset());
  EXPECT_EQ(10u, markers_intersecting_range[1]->EndOffset());
}

}  // namespace blink
