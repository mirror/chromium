// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FileSystemDirectoryReader_h
#define FileSystemDirectoryReader_h

#include "modules/entries/FileSystemDirectoryEntry.h"
#include "modules/entries/FileSystemEntriesCallback.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class ErrorCallback;

class FileSystemDirectoryReader : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static FileSystemDirectoryReader* Create(FileSystemDirectoryEntry* entry) {
    return new FileSystemDirectoryReader(entry);
  }

  void readEntries(FileSystemEntriesCallback*, ErrorCallback* = nullptr);

  FileSystem* Filesystem() { return entry_->filesystem(); }
  void SetHasMoreEntries(bool has_more_entries) {
    has_more_entries_ = has_more_entries;
  }

  virtual void Trace(blink::Visitor*);

 private:
  class EntriesCallbackHelper;
  class ErrorCallbackHelper;

  explicit FileSystemDirectoryReader(FileSystemDirectoryEntry*);

  // Called when entries are available. If |entries_callback_| is registered
  // it is invoked, otherwise the entries are stored in |entries_| until a
  // callback is passed.
  void AddEntries(const FileSystemEntryHeapVector& entries);

  // Called when an error is encountered. |error_| is set, |error_callback_|
  // is invoked if set, and future calls from script will produce this error.
  void OnError(FileError::ErrorCode);

  // The directory being read.
  Member<FileSystemDirectoryEntry> entry_;

  // Set to true after the first readEntries() call, which kicks off
  // the asynchronous enumeration.
  bool is_reading_ = false;

  // Set to false via SetHasMoreEntries() when the last response comes
  // in.
  bool has_more_entries_ = true;

  // Accumulates entries until the next |readEntries()| call from script.
  FileSystemEntryHeapVector entries_;

  FileError::ErrorCode error_ = FileError::ErrorCode::kOK;
  Member<FileSystemEntriesCallback> entries_callback_;
  Member<ErrorCallback> error_callback_;
};

}  // namespace blink

#endif  // FileSystemDirectoryReader_h
