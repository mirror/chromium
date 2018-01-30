// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/entries/FileSystemEntryCallbacks.h"

#include <memory>
#include "core/fileapi/File.h"
#include "modules/entries/FileCallback.h"
#include "modules/entries/FileSystemDirectoryReader.h"
#include "modules/entries/FileSystemEntriesCallback.h"
#include "modules/entries/FileSystemEntryCallback.h"
#include "modules/entries/FileSystemFileEntry.h"
#include "modules/filesystem/DOMFilePath.h"
#include "modules/filesystem/ErrorCallback.h"
#include "platform/AsyncFileSystemCallbacks.h"
#include "platform/FileMetadata.h"
#include "public/platform/WebFileSystemCallbacks.h"

namespace blink {

// FileSystemEntryCallbacksBase

FileSystemEntryCallbacksBase::~FileSystemEntryCallbacksBase() {
  if (file_system_)
    file_system_->RemovePendingCallbacks();
}

void FileSystemEntryCallbacksBase::DidFail(int code) {
  if (error_callback_) {
    InvokeOrScheduleCallback(error_callback_.Release(),
                             static_cast<FileError::ErrorCode>(code));
  }
}

FileSystemEntryCallbacksBase::FileSystemEntryCallbacksBase(
    FileSystemEntryErrorCallbackBase* error_callback,
    FileSystem* file_system,
    ExecutionContext* context)
    : error_callback_(error_callback),
      file_system_(file_system),
      execution_context_(context) {
  if (file_system_)
    file_system_->AddPendingCallbacks();
}

bool FileSystemEntryCallbacksBase::ShouldScheduleCallback() const {
  return execution_context_ && execution_context_->IsContextSuspended();
}

// Subclasses can call these to invoke the callback asynchronously
// (e.g. if the context is suspended) or synchronously as needed.
template <typename CB, typename CBArg>
void FileSystemEntryCallbacksBase::InvokeOrScheduleCallback(CB* callback,
                                                            CBArg&& arg) {
  DCHECK(callback);
  if (callback) {
    if (ShouldScheduleCallback()) {
      FileSystem::ScheduleCallback(
          execution_context_.Get(),
          WTF::Bind(&CB::Invoke, WrapPersistent(callback),
                    std::forward<CBArg>(arg)));
    } else {
      callback->Invoke(arg);
    }
  }
  execution_context_.Clear();
}

template <typename CB, typename CBArg>
void FileSystemEntryCallbacksBase::HandleEventOrScheduleCallback(CB* callback,
                                                                 CBArg* arg) {
  DCHECK(callback);
  if (callback) {
    if (ShouldScheduleCallback()) {
      FileSystem::ScheduleCallback(
          execution_context_.Get(),
          WTF::Bind(&CB::handleEvent, WrapPersistent(callback),
                    WrapPersistent(arg)));
    } else {
      callback->handleEvent(arg);
    }
  }
  execution_context_.Clear();
}

template <typename CB, typename CBArg>
void FileSystemEntryCallbacksBase::HandleEventOrScheduleCallback(CB* callback,
                                                                 CBArg&& arg) {
  DCHECK(callback);
  if (callback) {
    if (ShouldScheduleCallback()) {
      FileSystem::ScheduleCallback(
          execution_context_.Get(),
          WTF::Bind(&CB::handleEvent, WrapPersistent(callback),
                    WTF::Passed(std::forward<CBArg>(arg))));
    } else {
      callback->handleEvent(arg);
    }
  }
  execution_context_.Clear();
}

// FileSystemEntryCallbacks

FileSystemEntryCallbacks::FileSystemEntryCallbacks(
    FileSystemEntryCallback* success_callback,
    FileSystemEntryErrorCallbackBase* error_callback,
    ExecutionContext* context,
    FileSystem* file_system,
    const String& expected_path,
    bool is_directory)
    : FileSystemEntryCallbacksBase(error_callback, file_system, context),
      success_callback_(success_callback),
      expected_path_(expected_path),
      is_directory_(is_directory) {}

void FileSystemEntryCallbacks::DidSucceed() {
  if (success_callback_) {
    if (is_directory_) {
      HandleEventOrScheduleCallback(
          success_callback_.Release(),
          FileSystemDirectoryEntry::Create(file_system_, expected_path_));
    } else {
      HandleEventOrScheduleCallback(
          success_callback_.Release(),
          FileSystemFileEntry::Create(file_system_, expected_path_));
    }
  }
}

// FileSystemEntriesCallbacks

FileSystemEntriesCallbacks::FileSystemEntriesCallbacks(
    FileSystemEntriesCallback* success_callback,
    FileSystemEntryErrorCallbackBase* error_callback,
    ExecutionContext* context,
    FileSystemDirectoryReader* directory_reader,
    const String& base_path)
    : FileSystemEntryCallbacksBase(error_callback,
                                   directory_reader->Filesystem(),
                                   context),
      success_callback_(success_callback),
      directory_reader_(directory_reader),
      base_path_(base_path) {
  DCHECK(directory_reader_);
}

void FileSystemEntriesCallbacks::DidReadDirectoryEntry(const String& name,
                                                       bool is_directory) {
  if (is_directory) {
    entries_.push_back(FileSystemDirectoryEntry::Create(
        directory_reader_->Filesystem(),
        DOMFilePath::Append(base_path_, name)));
  } else {
    entries_.push_back(
        FileSystemFileEntry::Create(directory_reader_->Filesystem(),
                                    DOMFilePath::Append(base_path_, name)));
  }
}

void FileSystemEntriesCallbacks::DidReadDirectoryEntries(bool has_more) {
  directory_reader_->SetHasMoreEntries(has_more);
  PersistentHeapVector<Member<FileSystemEntry>> entries;
  entries.swap(entries_);
  if (success_callback_) {
    HandleEventOrScheduleCallback(success_callback_.Release(),
                                  std::move(entries));
  }
}

// SnapshotFileCallbacks

SnapshotFileCallbacks::SnapshotFileCallbacks(
    FileSystem* filesystem,
    const String& name,
    const KURL& url,
    FileCallback* success_callback,
    FileSystemEntryErrorCallbackBase* error_callback,
    ExecutionContext* context)
    : FileSystemEntryCallbacksBase(error_callback, filesystem, context),
      name_(name),
      url_(url),
      success_callback_(success_callback) {}

void SnapshotFileCallbacks::DidCreateSnapshotFile(
    const FileMetadata& metadata,
    RefPtr<BlobDataHandle> snapshot) {
  if (!success_callback_)
    return;

  // We can't directly use the snapshot blob data handle because the content
  // type on it hasn't been set.  The |snapshot| param is here to provide a a
  // chain of custody thru thread bridging that is held onto until *after*
  // we've coined a File with a new handle that has the correct type set on
  // it. This allows the blob storage system to track when a temp file can and
  // can't be safely deleted.

  // Drag/drop data is always backed by a native file.
  DCHECK(!metadata.platform_path.IsEmpty());

  HandleEventOrScheduleCallback(
      success_callback_.Release(),
      File::CreateForFileSystemFile(name_, metadata, File::kIsNotUserVisible));
}

// FileSystemEntryScriptErrorCallback

// static
FileSystemEntryScriptErrorCallback* FileSystemEntryScriptErrorCallback::Wrap(
    ErrorCallback* callback) {
  // FileSystem operations take an optional (nullable) callback. If a
  // script callback was not passed, don't bother creating a dummy wrapper
  // and checking during invoke().
  if (!callback)
    return nullptr;
  return new FileSystemEntryScriptErrorCallback(callback);
}

void FileSystemEntryScriptErrorCallback::Trace(blink::Visitor* visitor) {
  FileSystemEntryErrorCallbackBase::Trace(visitor);
  visitor->Trace(callback_);
}

void FileSystemEntryScriptErrorCallback::Invoke(FileError::ErrorCode error) {
  callback_->handleEvent(FileError::CreateDOMException(error));
}

}  // namespace blink
