// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EntryHeapVector_h
#define EntryHeapVector_h

#include "platform/heap/Handle.h"

namespace blink {

class FileSystemEntry;

using EntryHeapVector = HeapVector<Member<FileSystemEntry>>;

}  // namespace blink

#endif  // EntryHeapVector_h
