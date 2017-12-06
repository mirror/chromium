// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGCaretRect_h
#define NGCaretRect_h

#include "core/editing/Forward.h"

namespace blink {

// This file provides utility functions for computing caret rect in LayoutNG.

struct LocalCaretRect;

// TODO(xiaochengh): Documentation.
LocalCaretRect ComputeNGLocalCaretRect(const PositionWithAffinity&);
LocalCaretRect ComputeNGLocalCaretRect(const PositionInFlatTreeWithAffinity&);

}

#endif  // NGCaretRect_h
