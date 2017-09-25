// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AXRange_h
#define AXRange_h

#include <base/optional.h>

#include "core/editing/SelectionTemplate.h"
#include "core/editing/TextAffinity.h"
#include "modules/accessibility/AXPosition.h"
#include "platform/wtf/Allocator.h"

namespace blink {

class AXObject;

class AXRange {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

 public:
  AXRange() : anchor_(), focus_() {}

  AXRange(AXObject* anchor_object,
          int anchor_offset,
          AXObject* focus_object,
          int focus_offset,
          TextAffinity affinity = TextAffinity::kDownstream)
      : anchor_(AXPosition(anchor_object, anchor_offset, affinity)),
        focus_(AXPosition(focus_object, focus_offset, affinity)) {}

  template <typename Strategy>
  explicit AXRange(const SelectionTemplate<Strategy>&);

  AXRange(const AXRange&) = default;
  ~AXRange() = default;

  AXPosition AnchorPosition() const { return anchor_; }
  void SetAnchorPosition(const AXPosition& anchor) { anchor_ = anchor; }
  AXObject* AnchorObject() const { return anchor_.AnchorObject(); }
  base::Optional<int> AnchorOffset() const { return anchor_.Offset(); }

  AXPosition FocusPosition() const { return focus_; }
  void SetFocusPosition(const AXPosition& focus) { focus_ = focus; }
  AXObject* FocusObject() const { return focus_.AnchorObject(); }
  base::Optional<int> FocusOffset() const { return focus_.Offset(); }

  TextAffinity FocusAffinity() const { return focus_.Affinity(); }
  void SetFocusAffinity(TextAffinity affinity) { focus_.SetAffinity(affinity); }

  // The range is invalid if either the anchor or the focus position is invalid,
  // or if the positions are in two separate documents.
  bool IsValid() const;

  // Determines if the range only refers to text offsets under the current
  // object.
  bool IsSimple() const { return IsValid() && AnchorObject() == FocusObject(); }

  SelectionInDOMTree AsSelectionInDOMTree() const;

  // Tries to set the selection to this range.
  void Select();

 private:
  // The |AXPosition| where the range starts.
  AXPosition anchor_;

  // The |AXPosition| where the range ends.
  AXPosition focus_;
};

bool operator==(const AXRange& a, const AXRange& b) {
  if (!a.IsValid() || !b.IsValid())
    return false;
  return a.AnchorPosition() == b.AnchorPosition() &&
         a.FocusPosition() == b.FocusPosition();
}

bool operator!=(const AXRange& a, const AXRange& b) {
  return !(a == b);
}

}  // namespace blink

#endif  // AXRange_h
