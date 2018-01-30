// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/entries/FileSystemDirectoryReader.h"

#include "core/fileapi/FileError.h"
#include "modules/entries/FileSystemEntriesCallback.h"
#include "modules/entries/FileSystemEntry.h"
#include "modules/entries/FileSystemEntryCallbacks.h"
#include "modules/filesystem/ErrorCallback.h"

namespace blink {

class FileSystemDirectoryReader::EntriesCallbackHelper final
    : public FileSystemEntriesCallback {
 public:
  explicit EntriesCallbackHelper(FileSystemDirectoryReader* reader)
      : reader_(reader) {}

  void handleEvent(const FileSystemEntryHeapVector& entries) override {
    reader_->AddEntries(entries);
  }

  virtual void Trace(blink::Visitor* visitor) {
    visitor->Trace(reader_);
    FileSystemEntriesCallback::Trace(visitor);
  }

 private:
  // FIXME: This Member keeps the reader alive until all of the readDirectory
  // results are received. crbug.com/350285
  Member<FileSystemDirectoryReader> reader_;
};

class FileSystemDirectoryReader::ErrorCallbackHelper final
    : public FileSystemEntryErrorCallbackBase {
 public:
  explicit ErrorCallbackHelper(FileSystemDirectoryReader* reader)
      : reader_(reader) {}

  void Invoke(FileError::ErrorCode error) override { reader_->OnError(error); }

  virtual void Trace(blink::Visitor* visitor) {
    visitor->Trace(reader_);
    FileSystemEntryErrorCallbackBase::Trace(visitor);
  }

 private:
  Member<FileSystemDirectoryReader> reader_;
};

FileSystemDirectoryReader::FileSystemDirectoryReader(
    FileSystemDirectoryEntry* entry)
    : entry_(entry) {}

namespace {
void RunEntriesCallback(FileSystemEntriesCallback* callback,
                        HeapVector<Member<FileSystemEntry>>* entries) {
  callback->handleEvent(*entries);
}
}  // namespace

void FileSystemDirectoryReader::readEntries(
    FileSystemEntriesCallback* entries_callback,
    ErrorCallback* error_callback) {
  if (!is_reading_) {
    is_reading_ = true;
    entry_->filesystem()->ReadDirectory(this, entry_->fullPath(),
                                        new EntriesCallbackHelper(this),
                                        new ErrorCallbackHelper(this));
  }

  if (error_) {
    entry_->filesystem()->ScheduleErrorCallback(error_callback, error_);
    return;
  }

  if (entries_callback_) {
    // Non-null |entries_callback_| means multiple readEntries() calls are made
    // concurrently. We don't allow doing it.
    entry_->filesystem()->ScheduleErrorCallback(error_callback,
                                                FileError::kInvalidStateErr);
    return;
  }

  // If we're done, or if we have entries ready to go since the last
  // call, schedule delivery of them.
  if (!has_more_entries_ || !entries_.IsEmpty()) {
    if (entries_callback) {
      auto entries =
          new HeapVector<Member<FileSystemEntry>>(std::move(entries_));
      FileSystem::ScheduleCallback(
          entry_->filesystem()->GetExecutionContext(),
          WTF::Bind(&RunEntriesCallback, WrapPersistent(entries_callback),
                    WrapPersistent(entries)));
    }
    entries_.clear();
    return;
  }

  entries_callback_ = entries_callback;
  error_callback_ = error_callback;
}

void FileSystemDirectoryReader::AddEntries(
    const FileSystemEntryHeapVector& entries) {
  entries_.AppendVector(entries);
  error_callback_ = nullptr;
  if (entries_callback_) {
    FileSystemEntriesCallback* entries_callback = entries_callback_.Release();
    FileSystemEntryHeapVector entries;
    entries.swap(entries_);
    entries_callback->handleEvent(entries);
  }
}

void FileSystemDirectoryReader::OnError(FileError::ErrorCode error) {
  error_ = error;
  entries_callback_ = nullptr;
  if (error_callback_)
    error_callback_->handleEvent(FileError::CreateDOMException(error));
}

void FileSystemDirectoryReader::Trace(blink::Visitor* visitor) {
  ScriptWrappable::Trace(visitor);
  visitor->Trace(entry_);
  visitor->Trace(entries_);
  visitor->Trace(entries_callback_);
  visitor->Trace(error_callback_);
}

}  // namespace blink
