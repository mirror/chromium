// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FileSystemEntryCallbacks_h
#define FileSystemEntryCallbacks_h

#include "core/fileapi/FileError.h"
#include "platform/AsyncFileSystemCallbacks.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/PtrUtil.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class ErrorCallback;
class ExecutionContext;
class FileCallback;
class FileSystem;
class FileSystemDirectoryReader;
class FileSystemEntriesCallback;
class FileSystemEntry;
class FileSystemEntryCallback;
class FileSystemEntryErrorCallbackBase;

// Base Class --------------------------------------------------------------

class FileSystemEntryCallbacksBase : public AsyncFileSystemCallbacks {
  WTF_MAKE_NONCOPYABLE(FileSystemEntryCallbacksBase);

 public:
  ~FileSystemEntryCallbacksBase() override;

  // Invokes |error_callback_| if not null.
  void DidFail(int code) final;

  // Other AsyncFileSystemCallbacks methods are implemented by each subclass.

 protected:
  FileSystemEntryCallbacksBase(FileSystemEntryErrorCallbackBase*,
                               FileSystem*,
                               ExecutionContext*);

  bool ShouldScheduleCallback() const;

  // Subclasses can call these to invoke the callback asynchronously
  // (e.g. if the context is suspended) or synchronously as needed.
  template <typename CB, typename CBArg>
  void InvokeOrScheduleCallback(CB*, CBArg&&);

  template <typename CB, typename CBArg>
  void HandleEventOrScheduleCallback(CB*, CBArg*);

  template <typename CB, typename CBArg>
  void HandleEventOrScheduleCallback(CB*, CBArg&&);

  Persistent<FileSystemEntryErrorCallbackBase> error_callback_;
  Persistent<FileSystem> file_system_;
  Persistent<ExecutionContext> execution_context_;
};

// Subclasses --------------------------------------------------------------

// Bridge AsyncFileSystemCallbacks to FileSystemEntryCallback.
class FileSystemEntryCallbacks final : public FileSystemEntryCallbacksBase {
 public:
  FileSystemEntryCallbacks(FileSystemEntryCallback*,
                           FileSystemEntryErrorCallbackBase*,
                           ExecutionContext*,
                           FileSystem*,
                           const String& expected_path,
                           bool is_directory);

  void DidSucceed() override;

 private:
  Persistent<FileSystemEntryCallback> success_callback_;
  String expected_path_;
  bool is_directory_;
};

// Bridge AsyncFileSystemCallbacks to FileSystemEntriesCallback.
class FileSystemEntriesCallbacks final : public FileSystemEntryCallbacksBase {
 public:
  FileSystemEntriesCallbacks(FileSystemEntriesCallback*,
                             FileSystemEntryErrorCallbackBase*,
                             ExecutionContext*,
                             FileSystemDirectoryReader*,
                             const String& base_path);

  void DidReadDirectoryEntry(const String& name, bool is_directory) override;
  void DidReadDirectoryEntries(bool has_more) override;

 private:
  Persistent<FileSystemEntriesCallback> success_callback_;
  Persistent<FileSystemDirectoryReader> directory_reader_;
  String base_path_;
  PersistentHeapVector<Member<FileSystemEntry>> entries_;
};

// Bridge AsyncFileSystemCallbacks to FileCallback.
class SnapshotFileCallbacks final : public FileSystemEntryCallbacksBase {
 public:
  SnapshotFileCallbacks(FileSystem*,
                        const String& name,
                        const KURL&,
                        FileCallback*,
                        FileSystemEntryErrorCallbackBase*,
                        ExecutionContext*);

  void DidCreateSnapshotFile(const FileMetadata&,
                             RefPtr<BlobDataHandle>) override;

 private:
  String name_;
  KURL url_;
  Persistent<FileCallback> success_callback_;
};

// Error Callbacks ---------------------------------------------------------

class FileSystemEntryErrorCallbackBase
    : public GarbageCollectedFinalized<FileSystemEntryErrorCallbackBase> {
  WTF_MAKE_NONCOPYABLE(FileSystemEntryErrorCallbackBase);

 public:
  FileSystemEntryErrorCallbackBase() {}
  virtual ~FileSystemEntryErrorCallbackBase() {}
  virtual void Trace(blink::Visitor*) {}
  virtual void Invoke(FileError::ErrorCode) = 0;
};

// Bridge FileSystemEntryErrorCallbackBase to ErrorCallback.
class FileSystemEntryScriptErrorCallback final
    : public FileSystemEntryErrorCallbackBase {
 public:
  // Wrap an ErrorCallback passed from script. Since error callbacks are
  // optional in script, returns null if passed null to avoid an unnecessary
  // wrapper.
  static FileSystemEntryScriptErrorCallback* Wrap(ErrorCallback*);
  virtual ~FileSystemEntryScriptErrorCallback() {}

  virtual void Trace(blink::Visitor*);

  // Calls the wrapped callback with a DOMException.
  void Invoke(FileError::ErrorCode) override;

 private:
  explicit FileSystemEntryScriptErrorCallback(ErrorCallback* callback)
      : callback_(callback) {}
  Member<ErrorCallback> callback_;
};

}  // namespace blink

#endif  // FileSystemEntryCallbacks_h
