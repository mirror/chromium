// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AXRange_h
#define AXRange_h

#include "core/editing/Position.h"
#include "core/editing/SelectionTemplate.h"
#include "core/editing/TextAffinity.h"

namespace blink {

class AXObject;

class AXRange {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

 public:
  AXRange()
      : anchor_object_(nullptr),
        anchor_offset_(-1),
        focus_object_(nullptr),
        focus_offset_(-1),
        affinity_(TextAffinity::kDownstream) {}

  AXRange(AXObject* anchor_object,
          int anchor_offset,
          AXObject* focus_object,
          int focus_offset,
          TextAffinity affinity = TextAffinity::kDownstream)
      : anchor_object_(anchor_object),
        anchor_offset_(anchor_offset),
        focus_object_(focus_object),
        focus_offset_(focus_offset),
        affinity_(affinity) {}

  explicit AXRange(const SelectionInDOMTree&);
  explicit AXRange(const SelectionInFlatTree&);

  ~AXRange() = default;

  AXObject* AnchorObject() { return anchor_object_; }
  void SetAnchorObject(AXObject* anchor_object) {
    anchor_object_ = anchor_object;
  }
  int AnchorOffset() { return anchor_offset_; }
  void SetAnchorOffset(int anchor_offset) { anchor_offset_ = anchor_offset; }

  AXObject* FocusObject() { return focus_object_; }
  void SetFocusObject(AXObject* focus_object) { focus_object_ = focus_object; }
  int FocusOffset() { return focus_offset_; }
  void SetFocusOffset(int focus_offset) { focus_offset_ = focus_offset; }

  TextAffinity Affinity() { return affinity_; }
  void SetAffinity(TextAffinity affinity) { affinity_ = affinity; }

  bool IsValid() const;

  // Determines if the range only refers to text offsets under the current
  // object.
  bool IsSimple() const {
    return IsValid() && (anchor_object_ == focus_object_);
  }

  SelectionInDOMTree AsSelectionInDOMTree() const;

  // Tries to set the selection to this range.
  void Select();

 private:
  void Init(const PositionInFlatTree& base, const PositionInFlatTree& extent);

  // The deepest descendant in which the range starts.
  Persistent<AXObject> anchor_object_;
  // The number of characters or child objects in the anchor object before the
  // range starts.
  int anchor_offset_;

  // The deepest descendant in which the range ends.
  Persistent<AXObject> focus_object_;
  // The number of characters or child objects in the focus object before the
  // range ends.
  int focus_offset_;

  // When the same character offset could correspond to two possible caret
  // positions, upstream means it's on the previous line rather than the next
  // line.
  TextAffinity affinity_;
};

}  // namespace blink

#endif  // AXRange_h
