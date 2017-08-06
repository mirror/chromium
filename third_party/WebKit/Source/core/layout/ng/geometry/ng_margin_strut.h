// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGMarginStrut_h
#define NGMarginStrut_h

#include "core/CoreExport.h"
#include "platform/LayoutUnit.h"

namespace blink {

// This struct is used for the margin collapsing calculation.
struct CORE_EXPORT NGMarginStrut {
  LayoutUnit positive_margin;
  LayoutUnit negative_margin;

  // In quirks mode we additionally need to keep track of all the *non* quirky
  // margins.
  bool is_quirky_container_start = false;

  // Appends negative or positive value to the current margin strut.
  void Append(const LayoutUnit& value, bool is_quirky);

  // Sum up negative and positive margins of this strut.
  LayoutUnit Sum() const;

  String ToString() const;

  bool operator==(const NGMarginStrut& other) const;
};

CORE_EXPORT std::ostream& operator<<(std::ostream&, const NGMarginStrut&);

}  // namespace blink

#endif  // NGMarginStrut_h
