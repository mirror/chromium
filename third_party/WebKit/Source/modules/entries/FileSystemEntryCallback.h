// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FileSystemEntryCallback_h
#define FileSystemEntryCallback_h

#include "platform/heap/Handle.h"

namespace blink {

class FileSystemEntry;

class FileSystemEntryCallback
    : public GarbageCollectedFinalized<FileSystemEntryCallback> {
 public:
  virtual ~FileSystemEntryCallback() {}
  virtual void Trace(blink::Visitor*) {}
  virtual void handleEvent(FileSystemEntry*) = 0;
};

}  // namespace blink

#endif  // FileSystemEntryCallback_h
