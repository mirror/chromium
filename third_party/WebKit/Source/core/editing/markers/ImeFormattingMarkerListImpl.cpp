// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/markers/ImeFormattingMarkerListImpl.h"

#include "core/editing/markers/DocumentMarkerListEditor.h"

namespace blink {

DocumentMarker::MarkerType ImeFormattingMarkerListImpl::MarkerType() const {
  return DocumentMarker::kImeFormatting;
}

bool ImeFormattingMarkerListImpl::IsEmpty() const {
  return markers_.IsEmpty();
}

void ImeFormattingMarkerListImpl::Add(DocumentMarker* marker) {
  DocumentMarkerListEditor::AddMarkerWithoutMergingOverlapping(&markers_,
                                                               marker);
}

void ImeFormattingMarkerListImpl::Clear() {
  markers_.clear();
}

const HeapVector<Member<DocumentMarker>>&
ImeFormattingMarkerListImpl::GetMarkers() const {
  return markers_;
}

DocumentMarker* ImeFormattingMarkerListImpl::FirstMarkerIntersectingRange(
    unsigned start_offset,
    unsigned end_offset) const {
  return DocumentMarkerListEditor::FirstMarkerIntersectingRange(
      markers_, start_offset, end_offset);
}

HeapVector<Member<DocumentMarker>>
ImeFormattingMarkerListImpl::MarkersIntersectingRange(
    unsigned start_offset,
    unsigned end_offset) const {
  return DocumentMarkerListEditor::MarkersIntersectingRange(
      markers_, start_offset, end_offset);
}

bool ImeFormattingMarkerListImpl::MoveMarkers(
    int length,
    DocumentMarkerList* dst_markers_) {
  return DocumentMarkerListEditor::MoveMarkers(&markers_, length, dst_markers_);
}

bool ImeFormattingMarkerListImpl::RemoveMarkers(unsigned start_offset,
                                                int length) {
  return DocumentMarkerListEditor::RemoveMarkers(&markers_, start_offset,
                                                 length);
}

bool ImeFormattingMarkerListImpl::ShiftMarkers(const String&,
                                               unsigned offset,
                                               unsigned old_length,
                                               unsigned new_length) {
  return DocumentMarkerListEditor::ShiftMarkersContentIndependent(
      &markers_, offset, old_length, new_length);
}

DEFINE_TRACE(ImeFormattingMarkerListImpl) {
  visitor->Trace(markers_);
  DocumentMarkerList::Trace(visitor);
}

}  // namespace blink
