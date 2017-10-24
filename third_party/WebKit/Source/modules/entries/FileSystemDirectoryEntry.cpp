// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/entries/FileSystemDirectoryEntry.h"

#include "core/fileapi/FileError.h"
#include "modules/entries/FileSystemDirectoryReader.h"

namespace blink {

FileSystemDirectoryEntry::FileSystemDirectoryEntry(FileSystem* file_system,
                                                   const String& full_path)
    : FileSystemEntry(file_system, full_path) {}

FileSystemDirectoryReader* FileSystemDirectoryEntry::createReader() {
  return FileSystemDirectoryReader::Create(this);
}

void FileSystemDirectoryEntry::getFile(
    const String& path,
    const FileSystemFlags& options,
    FileSystemEntryCallback* success_callback,
    ErrorCallback* error_callback) {
  filesystem()->GetFile(this, path, options, success_callback, error_callback);
}

void FileSystemDirectoryEntry::getDirectory(
    const String& path,
    const FileSystemFlags& options,
    FileSystemEntryCallback* success_callback,
    ErrorCallback* error_callback) {
  filesystem()->GetDirectory(this, path, options, success_callback,
                             error_callback);
}

void FileSystemDirectoryEntry::Trace(blink::Visitor* visitor) {
  FileSystemEntry::Trace(visitor);
}

}  // namespace blink
