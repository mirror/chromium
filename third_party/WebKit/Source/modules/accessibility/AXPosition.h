// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AXPosition_h
#define AXPosition_h

#include <base/optional.h>

#include "core/editing/Position.h"
#include "core/editing/PositionWithAffinity.h"
#include "core/editing/TextAffinity.h"
#include "platform/wtf/Allocator.h"

namespace blink {

class AXObject;

// Describes a position in the Blink accessibility tree.
// Positions are made up of an |AXObject| and an optional child or text offset in that object.
class AXPosition {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

 public:
  AXPosition()
      : anchor_object_(nullptr),
        offset_(),
        affinity_(TextAffinity::kDownstream) {}

  AXPosition(AXObject* anchor_object,
          int offset,
          TextAffinity affinity = TextAffinity::kDownstream)
      : anchor_object_(anchor_object),
        offset_(offset),
        affinity_(affinity) {}

  template<typename Strategy> explicit AXPosition(const PositionTemplate<Strategy>&);

  template<typename Strategy> explicit AXPosition(const PositionWithAffinityTemplate<Strategy>& position) :
  AXPosition(position.GetPosition()), affinity_(position.Affinity()) {}

  AXPosition(const AXPosition&) = default;
  ~AXPosition() = default;

  AXObject* AnchorObject() const { return anchor_object_; }
  void SetAnchorObject(AXObject* anchor_object) { anchor_object_ = anchor_object; }

  base::Optional<int> Offset() const { return offset_; }
  void SetOffset(int offset) { offset_ = offset; }

  TextAffinity Affinity() const { return affinity_; }
  void SetAffinity(TextAffinity affinity) { affinity_ = affinity; }

  // Verifies if the anchor is present and if it's set to a live object with a connected node.
  bool IsValid() const;

  PositionInFlatTreeWithAffinity AsPositionInFlatTreeWithAffinity() const;

 private:
  // The |AXObject| in which the position is present.
  Persistent<AXObject> anchor_object_;

  // The number of characters or child objects in the anchor object before the
  // position.
  base::Optional<int> offset_;

  // When the same character offset could correspond to two possible caret
  // positions, upstream means it's on the previous line rather than the next
  // line.
  TextAffinity affinity_;
};

bool operator==(const AXPosition& a, const AXPosition& b) {
  if (!a.IsValid() || !b.IsValid())
    return false;
  return a.AnchorObject() == b.AnchorObject() && a.Offset() == b.Offset() && a.Affinity() == b.Affinity();
}

bool operator!=(const AXPosition& a, const AXPosition& b) {
  return !(a == b);
}

}  // namespace blink

#endif  // AXPosition_h
