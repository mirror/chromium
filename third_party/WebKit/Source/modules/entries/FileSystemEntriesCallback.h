// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FileSystemEntriesCallback_h
#define FileSystemEntriesCallback_h

#include "platform/heap/Handle.h"

namespace blink {

class FileSystemEntry;
using FileSystemEntryHeapVector = HeapVector<Member<FileSystemEntry>>;

// Implementation of FileSystemEntries callback interface.

class FileSystemEntriesCallback
    : public GarbageCollectedFinalized<FileSystemEntriesCallback> {
 public:
  virtual ~FileSystemEntriesCallback() {}
  virtual void Trace(blink::Visitor*) {}
  virtual void handleEvent(const FileSystemEntryHeapVector&) = 0;
};

}  // namespace blink

#endif  // FileSystemEntriesCallback_h
