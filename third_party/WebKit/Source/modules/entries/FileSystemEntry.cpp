// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/entries/FileSystemEntry.h"

#include "modules/filesystem/DOMFilePath.h"

namespace blink {

FileSystemEntry::FileSystemEntry(FileSystem* file_system,
                                 const String& full_path)
    : file_system_(file_system),
      full_path_(full_path),
      name_(DOMFilePath::GetName(full_path)) {}

FileSystemEntry::~FileSystemEntry() {}

void FileSystemEntry::getParent(FileSystemEntryCallback* success_callback,
                                ErrorCallback* error_callback) const {
  file_system_->GetParent(this, success_callback, error_callback);
}

void FileSystemEntry::Trace(blink::Visitor* visitor) {
  ScriptWrappable::Trace(visitor);
  visitor->Trace(file_system_);
}

}  // namespace blink
