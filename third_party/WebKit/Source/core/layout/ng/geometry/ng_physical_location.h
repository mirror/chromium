// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGPhysicalLocation_h
#define NGPhysicalLocation_h

#include "core/CoreExport.h"

#include "platform/LayoutUnit.h"

namespace blink {

class LayoutPoint;
struct NGPhysicalOffset;

// NGPhysicalLocation is the position of a rect (typically a fragment) relative
// to the root document.
struct CORE_EXPORT NGPhysicalLocation {
  NGPhysicalLocation() {}
  NGPhysicalLocation(LayoutUnit left, LayoutUnit top) : left(left), top(top) {}
  explicit NGPhysicalLocation(const NGPhysicalOffset&);
  LayoutUnit left;
  LayoutUnit top;

  bool operator==(const NGPhysicalLocation& other) const;

  // Conversions from/to existing code. New code prefers type safety for
  // logical/physical distinctions.
  LayoutPoint ToLayoutPoint() const;

  String ToString() const;
};

CORE_EXPORT std::ostream& operator<<(std::ostream&, const NGPhysicalLocation&);

}  // namespace blink

#endif  // NGPhysicalLocation_h
