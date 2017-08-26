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
       ContentIndependentMarker_ReplaceStartOfMarker) {
  UnsortedDocumentMarkerListEditor::MarkerList markers;
  markers.push_back(CreateMarker(0, 10));

  // Replace with shorter text
  UnsortedDocumentMarkerListEditor::ShiftMarkersContentIndependent(&markers, 0,
                                                                   5, 4);

  EXPECT_EQ(1u, markers.size());
  EXPECT_EQ(0u, markers[0]->StartOffset());
  EXPECT_EQ(9u, markers[0]->EndOffset());

  // Replace with longer text
  UnsortedDocumentMarkerListEditor::ShiftMarkersContentIndependent(&markers, 0,
                                                                   4, 5);

  EXPECT_EQ(1u, markers.size());
  EXPECT_EQ(0u, markers[0]->StartOffset());
  EXPECT_EQ(10u, markers[0]->EndOffset());

  // Replace with text of same length
  UnsortedDocumentMarkerListEditor::ShiftMarkersContentIndependent(&markers, 0,
                                                                   5, 5);

  EXPECT_EQ(1u, markers.size());
  EXPECT_EQ(0u, markers[0]->StartOffset());
  EXPECT_EQ(10u, markers[0]->EndOffset());
}

TEST_F(UnsortedDocumentMarkerListEditorTest,
       ContentIndependentMarker_ReplaceContainsStartOfMarker) {
  UnsortedDocumentMarkerListEditor::MarkerList markers;
  markers.push_back(CreateMarker(5, 15));

  UnsortedDocumentMarkerListEditor::ShiftMarkersContentIndependent(&markers, 0,
                                                                   10, 10);

  EXPECT_EQ(1u, markers.size());
  EXPECT_EQ(10u, markers[0]->StartOffset());
  EXPECT_EQ(15u, markers[0]->EndOffset());
}

TEST_F(UnsortedDocumentMarkerListEditorTest,
       ContentIndependentMarker_ReplaceEndOfMarker) {
  UnsortedDocumentMarkerListEditor::MarkerList markers;
  markers.push_back(CreateMarker(0, 10));

  // Replace with shorter text
  UnsortedDocumentMarkerListEditor::ShiftMarkersContentIndependent(&markers, 5,
                                                                   5, 4);

  EXPECT_EQ(1u, markers.size());
  EXPECT_EQ(0u, markers[0]->StartOffset());
  EXPECT_EQ(9u, markers[0]->EndOffset());

  // Replace with longer text
  UnsortedDocumentMarkerListEditor::ShiftMarkersContentIndependent(&markers, 5,
                                                                   4, 5);

  EXPECT_EQ(1u, markers.size());
  EXPECT_EQ(0u, markers[0]->StartOffset());
  EXPECT_EQ(10u, markers[0]->EndOffset());

  // Replace with text of same length
  UnsortedDocumentMarkerListEditor::ShiftMarkersContentIndependent(&markers, 5,
                                                                   5, 5);

  EXPECT_EQ(1u, markers.size());
  EXPECT_EQ(0u, markers[0]->StartOffset());
  EXPECT_EQ(10u, markers[0]->EndOffset());
}

TEST_F(UnsortedDocumentMarkerListEditorTest,
       ContentIndependentMarker_ReplaceContainsEndOfMarker) {
  UnsortedDocumentMarkerListEditor::MarkerList markers;
  markers.push_back(CreateMarker(0, 10));

  UnsortedDocumentMarkerListEditor::ShiftMarkersContentIndependent(&markers, 5,
                                                                   10, 10);

  EXPECT_EQ(1u, markers.size());
  EXPECT_EQ(0u, markers[0]->StartOffset());
  EXPECT_EQ(5u, markers[0]->EndOffset());
}

TEST_F(UnsortedDocumentMarkerListEditorTest,
       ContentIndependentMarker_ReplaceEntireMarker) {
  UnsortedDocumentMarkerListEditor::MarkerList markers;
  markers.push_back(CreateMarker(0, 10));

  // Replace with shorter text
  UnsortedDocumentMarkerListEditor::ShiftMarkersContentIndependent(&markers, 0,
                                                                   10, 9);

  EXPECT_EQ(1u, markers.size());
  EXPECT_EQ(0u, markers[0]->StartOffset());
  EXPECT_EQ(9u, markers[0]->EndOffset());

  // Replace with longer text
  UnsortedDocumentMarkerListEditor::ShiftMarkersContentIndependent(&markers, 0,
                                                                   9, 10);

  EXPECT_EQ(1u, markers.size());
  EXPECT_EQ(0u, markers[0]->StartOffset());
  EXPECT_EQ(10u, markers[0]->EndOffset());

  // Replace with text of same length
  UnsortedDocumentMarkerListEditor::ShiftMarkersContentIndependent(&markers, 0,
                                                                   10, 10);

  EXPECT_EQ(1u, markers.size());
  EXPECT_EQ(0u, markers[0]->StartOffset());
  EXPECT_EQ(10u, markers[0]->EndOffset());
}

TEST_F(UnsortedDocumentMarkerListEditorTest,
       ContentIndependentMarker_ReplaceTextWithMarkerAtBeginning) {
  UnsortedDocumentMarkerListEditor::MarkerList markers;
  markers.push_back(CreateMarker(0, 10));

  UnsortedDocumentMarkerListEditor::ShiftMarkersContentIndependent(&markers, 0,
                                                                   15, 15);

  EXPECT_EQ(0u, markers.size());
}

TEST_F(UnsortedDocumentMarkerListEditorTest,
       ContentIndependentMarker_ReplaceTextWithMarkerAtEnd) {
  UnsortedDocumentMarkerListEditor::MarkerList markers;
  markers.push_back(CreateMarker(5, 15));

  UnsortedDocumentMarkerListEditor::ShiftMarkersContentIndependent(&markers, 0,
                                                                   15, 15);

  EXPECT_EQ(0u, markers.size());
}

TEST_F(UnsortedDocumentMarkerListEditorTest,
       ContentIndependentMarker_Deletions) {
  UnsortedDocumentMarkerListEditor::MarkerList markers;
  markers.push_back(CreateMarker(0, 5));
  markers.push_back(CreateMarker(5, 10));
  markers.push_back(CreateMarker(10, 15));
  markers.push_back(CreateMarker(15, 20));
  markers.push_back(CreateMarker(20, 25));

  // Delete range containing the end of the second marker, the entire third
  // marker, and the start of the fourth marker
  UnsortedDocumentMarkerListEditor::ShiftMarkersContentIndependent(&markers, 8,
                                                                   9, 0);

  EXPECT_EQ(4u, markers.size());

  EXPECT_EQ(0u, markers[0]->StartOffset());
  EXPECT_EQ(5u, markers[0]->EndOffset());

  EXPECT_EQ(5u, markers[1]->StartOffset());
  EXPECT_EQ(8u, markers[1]->EndOffset());

  EXPECT_EQ(8u, markers[2]->StartOffset());
  EXPECT_EQ(11u, markers[2]->EndOffset());

  EXPECT_EQ(11u, markers[3]->StartOffset());
  EXPECT_EQ(16u, markers[3]->EndOffset());
}

TEST_F(UnsortedDocumentMarkerListEditorTest,
       ContentIndependentMarker_DeleteExactlyOnMarker) {
  UnsortedDocumentMarkerListEditor::MarkerList markers;
  markers.push_back(CreateMarker(0, 10));

  UnsortedDocumentMarkerListEditor::ShiftMarkersContentIndependent(&markers, 0,
                                                                   10, 0);

  EXPECT_EQ(0u, markers.size());
}

TEST_F(UnsortedDocumentMarkerListEditorTest,
       ContentIndependentMarker_InsertInMarkerInterior) {
  UnsortedDocumentMarkerListEditor::MarkerList markers;
  markers.push_back(CreateMarker(0, 5));
  markers.push_back(CreateMarker(5, 10));
  markers.push_back(CreateMarker(10, 15));

  // insert in middle of second marker
  UnsortedDocumentMarkerListEditor::ShiftMarkersContentIndependent(&markers, 7,
                                                                   0, 5);

  EXPECT_EQ(3u, markers.size());

  EXPECT_EQ(0u, markers[0]->StartOffset());
  EXPECT_EQ(5u, markers[0]->EndOffset());

  EXPECT_EQ(5u, markers[1]->StartOffset());
  EXPECT_EQ(15u, markers[1]->EndOffset());

  EXPECT_EQ(15u, markers[2]->StartOffset());
  EXPECT_EQ(20u, markers[2]->EndOffset());
}

TEST_F(UnsortedDocumentMarkerListEditorTest,
       ContentIndependentMarker_InsertBetweenMarkers) {
  UnsortedDocumentMarkerListEditor::MarkerList markers;
  markers.push_back(CreateMarker(0, 5));
  markers.push_back(CreateMarker(5, 10));
  markers.push_back(CreateMarker(10, 15));

  // insert before second marker
  UnsortedDocumentMarkerListEditor::ShiftMarkersContentIndependent(&markers, 5,
                                                                   0, 5);

  EXPECT_EQ(3u, markers.size());

  EXPECT_EQ(0u, markers[0]->StartOffset());
  EXPECT_EQ(5u, markers[0]->EndOffset());

  EXPECT_EQ(10u, markers[1]->StartOffset());
  EXPECT_EQ(15u, markers[1]->EndOffset());

  EXPECT_EQ(15u, markers[2]->StartOffset());
  EXPECT_EQ(20u, markers[2]->EndOffset());
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
       FirstMarkerIntersectingRange_EmptyRange) {
  UnsortedDocumentMarkerListEditor::MarkerList markers;
  markers.push_back(CreateMarker(0, 5));
  DocumentMarker* marker =
      UnsortedDocumentMarkerListEditor::FirstMarkerIntersectingRange(markers, 5,
                                                                     15);
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

TEST_F(UnsortedDocumentMarkerListEditorTest, MarkersIntersectingRange_Empty) {
  UnsortedDocumentMarkerListEditor::MarkerList markers;
  markers.push_back(CreateMarker(0, 5));

  UnsortedDocumentMarkerListEditor::MarkerList markers_intersecting_range =
      UnsortedDocumentMarkerListEditor::MarkersIntersectingRange(markers, 10,
                                                                 15);
  EXPECT_EQ(0u, markers_intersecting_range.size());
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

TEST_F(UnsortedDocumentMarkerListEditorTest,
       MarkersIntersectingRange_CollapsedRange) {
  UnsortedDocumentMarkerListEditor::MarkerList markers;
  markers.push_back(CreateMarker(5, 10));

  UnsortedDocumentMarkerListEditor::MarkerList markers_intersecting_range =
      UnsortedDocumentMarkerListEditor::MarkersIntersectingRange(markers, 7, 7);
  EXPECT_EQ(1u, markers_intersecting_range.size());

  EXPECT_EQ(5u, markers_intersecting_range[0]->StartOffset());
  EXPECT_EQ(10u, markers_intersecting_range[0]->EndOffset());
}

}  // namespace blink
