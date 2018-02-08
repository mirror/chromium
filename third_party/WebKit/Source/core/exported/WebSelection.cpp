// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/web/WebSelection.h"
#include "core/editing/SelectionType.h"

namespace blink {

WebSelection::WebSelection()
    : selection_type_(kNoSelection), start_({}), end_({}) {}

WebSelection::WebSelection(SelectionType type,
                           WebSelectionBound start,
                           WebSelectionBound end)
    : selection_type_(type), start_(start), end_(end) {}

// SelectionType enums have the same values; enforced in
// AssertMatchingEnums.cpp.

WebSelection::WebSelection(const WebSelection& other) = default;

}  // namespace blink
