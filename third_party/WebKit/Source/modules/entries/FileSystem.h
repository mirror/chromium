// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FileSystem_h
#define FileSystem_h

#include "core/dom/ContextLifecycleObserver.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/TaskRunnerHelper.h"
#include "core/fileapi/FileError.h"
#include "modules/ModulesExport.h"
#include "platform/bindings/ActiveScriptWrappable.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/PtrUtil.h"
#include "platform/wtf/text/WTFString.h"
#include "public/platform/WebTraceLocation.h"

namespace blink {

class ErrorCallback;
class FileCallback;
class FileSystemDirectoryEntry;
class FileSystemDirectoryReader;
class FileSystemEntriesCallback;
class FileSystemEntry;
class FileSystemEntryCallback;
class FileSystemEntryErrorCallbackBase;
class FileSystemFileEntry;
class FileSystemFlags;

class MODULES_EXPORT FileSystem final
    : public ScriptWrappable,
      public ActiveScriptWrappable<FileSystem>,
      public ContextClient {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(FileSystem);

 public:
  static const char kIsolatedPathPrefix[];

  static FileSystem* Create(ExecutionContext*, const String& filesystem_id);
  virtual ~FileSystem();

  // FileSystem.idl implementation.
  const String& name() const { return name_; }
  FileSystemDirectoryEntry* root() const { return root_entry_; }

  // Methods used by callbacks to control lifetime and report errors.
  void AddPendingCallbacks();
  void RemovePendingCallbacks();
  void ScheduleErrorCallback(ErrorCallback*, FileError::ErrorCode);
  void ScheduleErrorCallback(FileSystemEntryErrorCallbackBase*,
                             FileError::ErrorCode);
  static void ScheduleErrorCallback(ExecutionContext*,
                                    FileSystemEntryErrorCallbackBase*,
                                    FileError::ErrorCode);

  // ScriptWrappable overrides.
  bool HasPendingActivity() const final;

  // Schedule a callback. This should not cross threads (should be called on the
  // same context thread).
  static void ScheduleCallback(ExecutionContext*, WTF::Closure task);

  // Methods for FileSystemEntry, FileSystemFileEntry,
  // FileSystemDirectoryEntry, and FileSystemDirectoryReader
  // to avoid a dependency on WebFileSystem.
  void GetParent(const FileSystemEntry*,
                 FileSystemEntryCallback*,
                 ErrorCallback*);
  void GetFile(const FileSystemEntry*,
               const String& path,
               const FileSystemFlags&,
               FileSystemEntryCallback*,
               ErrorCallback*);
  void GetDirectory(const FileSystemEntry*,
                    const String& path,
                    const FileSystemFlags&,
                    FileSystemEntryCallback*,
                    ErrorCallback*);
  int ReadDirectory(FileSystemDirectoryReader*,
                    const String& path,
                    FileSystemEntriesCallback*,
                    FileSystemEntryErrorCallbackBase*);
  void SnapshotFile(const FileSystemFileEntry*, FileCallback*, ErrorCallback*);

  virtual void Trace(blink::Visitor*);

 private:
  FileSystem(ExecutionContext*, const String& name, const KURL&);

  static String TaskNameForInstrumentation() { return "FileSystem"; }

  KURL CreateFileSystemURL(const String& full_path) const;
  static bool PathToAbsolutePath(const FileSystemEntry*,
                                 String path,
                                 String& absolute_path);

  const String name_;
  const KURL filesystem_root_url_;
  int number_of_pending_callbacks_ = 0;
  Member<FileSystemDirectoryEntry> root_entry_;
};

}  // namespace blink

#endif  // FileSystem_h
