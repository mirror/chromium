/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FileSystemCallbacks_h
#define FileSystemCallbacks_h

#include <memory>
#include "core/fileapi/FileError.h"
#include "platform/AsyncFileSystemCallbacks.h"
#include "platform/FileSystemType.h"
#include "platform/wtf/Vector.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class CallbackInterfaceCollection;
class DOMFileSystemBase;
class DirectoryReaderBase;
class Entry;
class ExecutionContext;
class FileMetadata;
class FileWriterBase;
class FileWriterBaseCallback;
class V8BlobCallback;
class V8EntriesCallback;
class V8EntryCallback;
class V8ErrorCallback;
class V8FileSystemCallback;
class V8MetadataCallback;
class V8VoidCallback;

// Passed to DOMFileSystem implementations that may report errors. Subclasses
// may capture the error for throwing on return to script (for synchronous APIs)
// or call an actual script callback (for asynchronous APIs).
class ErrorCallbackBase : public GarbageCollectedFinalized<ErrorCallbackBase> {
 public:
  virtual ~ErrorCallbackBase() {}
  DEFINE_INLINE_VIRTUAL_TRACE() {}
  virtual void Invoke(FileError::ErrorCode) = 0;
};

class FileSystemCallbacksBase : public AsyncFileSystemCallbacks {
 public:
  ~FileSystemCallbacksBase() override;

  // For ErrorCallback.
  void DidFail(int code) final;

  // Other callback methods are implemented by each subclass.

 protected:
  FileSystemCallbacksBase(ErrorCallbackBase*,
                          DOMFileSystemBase*,
                          ExecutionContext*);

  bool ShouldScheduleCallback() const;

  template <typename CallbackType, typename ...Args>
  void CallOrScheduleCallbackHandleEvent(CallbackType*, ScriptWrappable*, Args...);

  template <typename CB, typename CBArg>
  void InvokeOrScheduleCallback(CB*, CBArg);

  template <typename CB, typename CBArg>
  void HandleEventOrScheduleCallback(CB*, CBArg*);

  template <typename CB>
  void HandleEventOrScheduleCallback(CB*);

  Persistent<ErrorCallbackBase> error_callback_;
  Persistent<DOMFileSystemBase> file_system_;
  Persistent<ExecutionContext> execution_context_;
  int async_operation_id_;
};

// Subclasses ----------------------------------------------------------------

// Wraps a script-provided callback for use in DOMFileSystem operations.
class ScriptErrorCallback final : public ErrorCallbackBase {
 public:
  static ScriptErrorCallback* Wrap(V8ErrorCallback*, ExecutionContext*);
  ~ScriptErrorCallback() override;
  DECLARE_VIRTUAL_TRACE();

  void Invoke(FileError::ErrorCode) override;

 private:
  ScriptErrorCallback(V8ErrorCallback*, ExecutionContext*);
  Member<V8ErrorCallback> callback_;
  WeakMember<CallbackInterfaceCollection> keep_alive_host_;
};

class EntryCallbacks final : public FileSystemCallbacksBase {
 public:
  static std::unique_ptr<AsyncFileSystemCallbacks> Create(
      V8EntryCallback*,
      ErrorCallbackBase*,
      ExecutionContext*,
      DOMFileSystemBase*,
      const String& expected_path,
      bool is_directory);
  ~EntryCallbacks() override;
  void DidSucceed() override;

 private:
  EntryCallbacks(V8EntryCallback*,
                 ErrorCallbackBase*,
                 ExecutionContext*,
                 DOMFileSystemBase*,
                 const String& expected_path,
                 bool is_directory);
  Persistent<V8EntryCallback> success_callback_;
  String expected_path_;
  bool is_directory_;
};

class EntriesCallbacks final : public FileSystemCallbacksBase {
 public:
  static std::unique_ptr<AsyncFileSystemCallbacks> Create(
      V8EntriesCallback*,
      ErrorCallbackBase*,
      ExecutionContext*,
      DirectoryReaderBase*,
      const String& base_path);
  ~EntriesCallbacks() override;
  void DidReadDirectoryEntry(const String& name, bool is_directory) override;
  void DidReadDirectoryEntries(bool has_more) override;

 private:
  EntriesCallbacks(V8EntriesCallback*,
                   ErrorCallbackBase*,
                   ExecutionContext*,
                   DirectoryReaderBase*,
                   const String& base_path);
  Persistent<V8EntriesCallback> success_callback_;
  Persistent<DirectoryReaderBase> directory_reader_;
  String base_path_;
  PersistentHeapVector<Member<Entry>> entries_;
};

class FileSystemCallbacks final : public FileSystemCallbacksBase {
 public:
  static std::unique_ptr<AsyncFileSystemCallbacks> Create(V8FileSystemCallback*,
                                                          ErrorCallbackBase*,
                                                          ExecutionContext*,
                                                          FileSystemType);
  ~FileSystemCallbacks() override;
  void DidOpenFileSystem(const String& name, const KURL& root_url) override;

 private:
  FileSystemCallbacks(V8FileSystemCallback*,
                      ErrorCallbackBase*,
                      ExecutionContext*,
                      FileSystemType);
  Persistent<V8FileSystemCallback> success_callback_;
  FileSystemType type_;
};

class ResolveURICallbacks final : public FileSystemCallbacksBase {
 public:
  static std::unique_ptr<AsyncFileSystemCallbacks> Create(V8EntryCallback*,
                                                          ErrorCallbackBase*,
                                                          ExecutionContext*);
  void DidResolveURL(const String& name,
                     const KURL& root_url,
                     FileSystemType,
                     const String& file_path,
                     bool is_directry) override;

 private:
  ResolveURICallbacks(V8EntryCallback*, ErrorCallbackBase*, ExecutionContext*);
  Persistent<V8EntryCallback> success_callback_;
};

class MetadataCallbacks final : public FileSystemCallbacksBase {
 public:
  static std::unique_ptr<AsyncFileSystemCallbacks> Create(V8MetadataCallback*,
                                                          ErrorCallbackBase*,
                                                          ExecutionContext*,
                                                          DOMFileSystemBase*);
  void DidReadMetadata(const FileMetadata&) override;

 private:
  MetadataCallbacks(V8MetadataCallback*,
                    ErrorCallbackBase*,
                    ExecutionContext*,
                    DOMFileSystemBase*);
  Persistent<V8MetadataCallback> success_callback_;
};

class FileWriterBaseCallbacks final : public FileSystemCallbacksBase {
 public:
  static std::unique_ptr<AsyncFileSystemCallbacks> Create(
      FileWriterBase*,
      FileWriterBaseCallback*,
      ErrorCallbackBase*,
      ExecutionContext*);
  void DidCreateFileWriter(std::unique_ptr<WebFileWriter>,
                           long long length) override;

 private:
  FileWriterBaseCallbacks(FileWriterBase*,
                          FileWriterBaseCallback*,
                          ErrorCallbackBase*,
                          ExecutionContext*);
  Persistent<FileWriterBase> file_writer_;
  Persistent<FileWriterBaseCallback> success_callback_;
};

class SnapshotFileCallback final : public FileSystemCallbacksBase {
 public:
  static std::unique_ptr<AsyncFileSystemCallbacks> Create(DOMFileSystemBase*,
                                                          const String& name,
                                                          const KURL&,
                                                          V8BlobCallback*,
                                                          ErrorCallbackBase*,
                                                          ExecutionContext*);
  virtual void DidCreateSnapshotFile(const FileMetadata&,
                                     PassRefPtr<BlobDataHandle> snapshot);

 private:
  SnapshotFileCallback(DOMFileSystemBase*,
                       const String& name,
                       const KURL&,
                       V8BlobCallback*,
                       ErrorCallbackBase*,
                       ExecutionContext*);
  String name_;
  KURL url_;
  Persistent<V8BlobCallback> success_callback_;
};

class VoidCallbacks final : public FileSystemCallbacksBase {
 public:
  static std::unique_ptr<AsyncFileSystemCallbacks> Create(V8VoidCallback*,
                                                          ErrorCallbackBase*,
                                                          ExecutionContext*,
                                                          DOMFileSystemBase*);
  void DidSucceed() override;

 private:
  VoidCallbacks(V8VoidCallback*,
                ErrorCallbackBase*,
                ExecutionContext*,
                DOMFileSystemBase*);
  Persistent<V8VoidCallback> success_callback_;
};

}  // namespace blink

#endif  // FileSystemCallbacks_h
