// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/filesystem/EntryHeapVector.h"

#include "modules/filesystem/Entry.h"

namespace blink {

void EntryHeapVectorCarrier::Trace(blink::Visitor* visitor) {
  visitor->Trace(entries_);
}

}  // namespace blink
