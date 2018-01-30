// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/entries/FileSystemFileEntry.h"

#include "modules/entries/FileCallback.h"
#include "modules/entries/FileSystem.h"

namespace blink {

FileSystemFileEntry::FileSystemFileEntry(FileSystem* file_system,
                                         const String& full_path)
    : FileSystemEntry(file_system, full_path) {}

void FileSystemFileEntry::file(FileCallback* success_callback,
                               ErrorCallback* error_callback) const {
  filesystem()->SnapshotFile(this, success_callback, error_callback);
}

void FileSystemFileEntry::Trace(blink::Visitor* visitor) {
  FileSystemEntry::Trace(visitor);
}

}  // namespace blink
