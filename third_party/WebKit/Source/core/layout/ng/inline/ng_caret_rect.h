// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGCaretRect_h
#define NGCaretRect_h

#include "core/editing/Forward.h"

namespace blink {

// This file provides utility functions for computing caret rect in LayoutNG.

class LayoutBlockFlow;
struct LocalCaretRect;

// TODO(xiaochengh): Documentation.
LocalCaretRect ComputeNGLocalCaretRect(const LayoutBlockFlow&,
                                       const PositionWithAffinity&);
LocalCaretRect ComputeNGLocalCaretRect(const LayoutBlockFlow&,
                                       const PositionInFlatTreeWithAffinity&);

}  // namespace blink

#endif  // NGCaretRect_h
