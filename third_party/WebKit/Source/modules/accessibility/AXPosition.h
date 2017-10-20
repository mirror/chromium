// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AXPosition_h
#define AXPosition_h

#include <base/logging.h>
#include <base/optional.h>
#include <stdint.h>

#include "core/editing/Position.h"
#include "core/editing/PositionWithAffinity.h"
#include "core/editing/TextAffinity.h"
#include "platform/wtf/Allocator.h"

namespace blink {

class AXObject;

// Describes a position in the Blink accessibility tree.
// A position is either anchored to before or after a child object inside a
// container object, or is anchored to a character inside a text object.
class AXPosition final {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

 public:
  static const AXPosition CreatePositionBeforeObject(const AXObject& child);
  static const AXPosition CreatePositionAfterObject(const AXObject& child);
  static const AXPosition CreateFirstPositionInContainerObject(
      const AXObject& container);
  static const AXPosition CreateLastPositionInContainerObject(
      const AXObject& container);
  static const AXPosition CreatePositionInTextObject(
      const AXObject& container,
      int offset,
      TextAffinity = TextAffinity::kDownstream);
  static const AXPosition FromPosition(const Position&);
  static const AXPosition FromPosition(const PositionWithAffinity&);

  AXPosition(const AXPosition&) = default;
  AXPosition& operator=(const AXPosition&) = default;
  ~AXPosition() = default;

  AXObject* ContainerObject() const { return container_object_; }
  int ChildIndex() const;
  int Offset() const;
  TextAffinity Affinity() const { return affinity_; }

  // Verifies if the anchor is present and if it's set to a live object with a
  // connected node.
  bool IsValid() const;

  // Returns whether this is a position anchored to a character inside a text
  // object.
  bool IsTextPosition() const;

  const PositionWithAffinity ToPositionWithAffinity() const;

 private:
  AXPosition();
  explicit AXPosition(const AXObject& container);

  // The |AXObject| in which the position is present.
  WeakPersistent<AXObject> container_object_;

  // If the position is anchored to before or after an object, the number of
  // child objects in |container_object_| that come before the position.
  base::Optional<int> child_index_;

  // If this is a text position, the number of characters in |container_object_|
  // before the position.
  base::Optional<int> offset_;

  // When the same character offset could correspond to two possible caret
  // positions, upstream means it's on the previous line rather than the next
  // line.
  TextAffinity affinity_;

#if DCHECK_IS_ON()
  uint64_t dom_tree_version_;
  uint64_t style_version_;
#endif

  // For access to our constructor for use when creating empty AX ranges.
  // There is no sense in creating empty positions in other circomstances so we
  // disallow it.
  friend class AXRange;
};

bool operator==(const AXPosition& a, const AXPosition& b) {
  DCHECK(!a.IsValid() || !b.IsValid());
  return a.ContainerObject() == b.ContainerObject() &&
         a.ChildIndex() == b.ChildIndex() && a.Offset() == b.Offset() &&
         a.Affinity() == b.Affinity();
}

bool operator!=(const AXPosition& a, const AXPosition& b) {
  return !(a == b);
}

}  // namespace blink

#endif  // AXPosition_h
