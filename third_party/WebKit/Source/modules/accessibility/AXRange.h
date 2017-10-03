// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AXRange_h
#define AXRange_h

#include <base/logging.h>
#include <base/optional.h>
#include <stdint.h>

#include "core/editing/SelectionTemplate.h"
#include "core/editing/TextAffinity.h"
#include "modules/accessibility/AXPosition.h"
#include "platform/wtf/Allocator.h"

namespace blink {

class AXRange final {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

 public:
  class Builder;

  AXRange(const AXRange&) = default;
  AXRange& operator=(const AXRange&) = default;
  ~AXRange() = default;

  const AXPosition Base() const { return base_; }
  const AXPosition Extent() const { return extent_; }

  // The range is invalid if either the anchor or the focus position is invalid,
  // or if the positions are in two separate documents.
  bool IsValid() const;

  // Determines if the range only refers to text offsets under the current
  // object.
  bool IsSimple() const {
    return IsValid() && base_.ContainerObject() == extent_.ContainerObject();
  }

  const SelectionInDOMTree AsSelection() const;

  // Tries to set the selection to this range.
  void Select();

 private:
  AXRange();

  // The |AXPosition| where the range starts.
  AXPosition base_;

  // The |AXPosition| where the range ends.
  AXPosition extent_;

#if DCHECK_IS_ON()
  uint64_t dom_tree_version_;
  uint64_t style_version_;
#endif

  friend class Builder;
};

class AXRange::Builder final {
  STACK_ALLOCATED();

 public:
  Builder() = default;
  ~Builder() = default;
  Builder& SetBase(const AXPosition&);
  Builder& SetBase(const Position&);
  Builder& SetExtent(const AXPosition&);
  Builder& SetExtent(const Position&);
  Builder& SetRange(const SelectionInDOMTree&);
  const AXRange Build();

 private:
  AXRange range_;
};

bool operator==(const AXRange& a, const AXRange& b) {
  DCHECK(!a.IsValid() || !b.IsValid());
  return a.Base() == b.Base() && a.Extent() == b.Extent();
}

bool operator!=(const AXRange& a, const AXRange& b) {
  return !(a == b);
}

}  // namespace blink

#endif  // AXRange_h
