// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/entries/FileSystem.h"

#include <memory>
#include "core/probe/CoreProbes.h"
#include "modules/entries/FileSystemDirectoryEntry.h"
#include "modules/entries/FileSystemDirectoryReader.h"
#include "modules/entries/FileSystemEntriesCallback.h"
#include "modules/entries/FileSystemEntryCallback.h"
#include "modules/entries/FileSystemEntryCallbacks.h"
#include "modules/entries/FileSystemFileEntry.h"
#include "modules/filesystem/DOMFilePath.h"
#include "modules/filesystem/ErrorCallback.h"
#include "modules/filesystem/FileSystemFlags.h"
#include "platform/WebTaskRunner.h"
#include "platform/weborigin/SecurityOrigin.h"
#include "platform/wtf/PtrUtil.h"
#include "platform/wtf/text/StringBuilder.h"
#include "public/platform/Platform.h"
#include "public/platform/WebFileSystem.h"
#include "public/platform/WebFileSystemCallbacks.h"
#include "public/platform/WebSecurityOrigin.h"

namespace blink {

namespace {

WebFileSystem* GetWebFileSystem() {
  Platform* platform = Platform::Current();
  if (!platform)
    return nullptr;
  return platform->FileSystem();
}

void RunCallback(ExecutionContext* execution_context,
                 WTF::Closure task,
                 std::unique_ptr<int> identifier) {
  if (!execution_context)
    return;
  DCHECK(execution_context->IsContextThread());
  probe::AsyncTask async_task(execution_context, identifier.get());
  task();
}

}  // namespace

const char FileSystem::kIsolatedPathPrefix[] = "isolated";

// static
FileSystem* FileSystem::Create(ExecutionContext* context,
                               const String& filesystem_id) {
  if (filesystem_id.IsEmpty())
    return nullptr;

  StringBuilder filesystem_name;
  filesystem_name.Append(Platform::Current()->FileSystemCreateOriginIdentifier(
      WebSecurityOrigin(context->GetSecurityOrigin())));
  filesystem_name.Append(":Isolated_");
  filesystem_name.Append(filesystem_id);

  // The rootURL created here is going to be attached to each filesystem request
  // and is to be validated each time the request is being handled.
  StringBuilder root_url;
  root_url.Append("filesystem:");
  root_url.Append(context->GetSecurityOrigin()->ToString());
  root_url.Append('/');
  root_url.Append(kIsolatedPathPrefix);
  root_url.Append('/');
  root_url.Append(filesystem_id);
  root_url.Append('/');

  return new FileSystem(context, filesystem_name.ToString(),
                        KURL(root_url.ToString()));
}

FileSystem::FileSystem(ExecutionContext* context,
                       const String& name,
                       const KURL& url)
    : ContextClient(context),
      name_(name),
      filesystem_root_url_(url),
      root_entry_(FileSystemDirectoryEntry::Create(this, DOMFilePath::kRoot)) {}

FileSystem::~FileSystem() {}

void FileSystem::AddPendingCallbacks() {
  ++number_of_pending_callbacks_;
}

void FileSystem::RemovePendingCallbacks() {
  DCHECK_GT(number_of_pending_callbacks_, 0);
  --number_of_pending_callbacks_;
}

bool FileSystem::HasPendingActivity() const {
  DCHECK_GE(number_of_pending_callbacks_, 0);
  return number_of_pending_callbacks_;
}

void FileSystem::ScheduleErrorCallback(ErrorCallback* error_callback,
                                       FileError::ErrorCode file_error) {
  ScheduleErrorCallback(
      GetExecutionContext(),
      FileSystemEntryScriptErrorCallback::Wrap(error_callback), file_error);
}

void FileSystem::ScheduleErrorCallback(
    FileSystemEntryErrorCallbackBase* error_callback,
    FileError::ErrorCode file_error) {
  ScheduleErrorCallback(GetExecutionContext(), error_callback, file_error);
}

void FileSystem::ScheduleErrorCallback(
    ExecutionContext* execution_context,
    FileSystemEntryErrorCallbackBase* error_callback,
    FileError::ErrorCode file_error) {
  if (!error_callback)
    return;
  ScheduleCallback(execution_context,
                   WTF::Bind(&FileSystemEntryErrorCallbackBase::Invoke,
                             WrapPersistent(error_callback), file_error));
}

void FileSystem::ScheduleCallback(ExecutionContext* execution_context,
                                  WTF::Closure task) {
  DCHECK(execution_context->IsContextThread());

  std::unique_ptr<int> identifier = WTF::MakeUnique<int>(0);
  probe::AsyncTaskScheduled(execution_context, TaskNameForInstrumentation(),
                            identifier.get());
  TaskRunnerHelper::Get(TaskType::kFileReading, execution_context)
      ->PostTask(BLINK_FROM_HERE,
                 WTF::Bind(&RunCallback, WrapWeakPersistent(execution_context),
                           WTF::Passed(std::move(task)),
                           WTF::Passed(std::move(identifier))));
}

void FileSystem::GetParent(const FileSystemEntry* entry,
                           FileSystemEntryCallback* success_callback,
                           ErrorCallback* error_callback) {
  if (!GetExecutionContext())
    return;

  if (!GetWebFileSystem()) {
    ScheduleErrorCallback(error_callback, FileError::kAbortErr);
    return;
  }

  DCHECK(entry);
  String path = DOMFilePath::GetDirectory(entry->fullPath());

  std::unique_ptr<AsyncFileSystemCallbacks> callbacks =
      WTF::MakeUnique<FileSystemEntryCallbacks>(
          success_callback,
          FileSystemEntryScriptErrorCallback::Wrap(error_callback),
          GetExecutionContext(), this, path, true);
  GetWebFileSystem()->DirectoryExists(CreateFileSystemURL(path),
                                      std::move(callbacks));
}

void FileSystem::GetFile(const FileSystemEntry* entry,
                         const String& path,
                         const FileSystemFlags& flags,
                         FileSystemEntryCallback* success_callback,
                         ErrorCallback* error_callback) {
  if (!GetExecutionContext())
    return;

  if (!GetWebFileSystem()) {
    ScheduleErrorCallback(error_callback, FileError::kAbortErr);
    return;
  }

  String absolute_path;
  if (!PathToAbsolutePath(entry, path, absolute_path)) {
    ScheduleErrorCallback(error_callback, FileError::kTypeMismatchErr);
    return;
  }

  if (flags.createFlag()) {
    ScheduleErrorCallback(error_callback, FileError::kSecurityErr);
    return;
  }

  std::unique_ptr<AsyncFileSystemCallbacks> callbacks =
      WTF::MakeUnique<FileSystemEntryCallbacks>(
          success_callback,
          FileSystemEntryScriptErrorCallback::Wrap(error_callback),
          GetExecutionContext(), this, absolute_path, false);
  GetWebFileSystem()->FileExists(CreateFileSystemURL(absolute_path),
                                 std::move(callbacks));
}

void FileSystem::GetDirectory(const FileSystemEntry* entry,
                              const String& path,
                              const FileSystemFlags& flags,
                              FileSystemEntryCallback* success_callback,
                              ErrorCallback* error_callback) {
  if (!GetExecutionContext())
    return;

  if (!GetWebFileSystem()) {
    ScheduleErrorCallback(error_callback, FileError::kAbortErr);
    return;
  }

  String absolute_path;
  if (!PathToAbsolutePath(entry, path, absolute_path)) {
    ScheduleErrorCallback(error_callback, FileError::kTypeMismatchErr);
    return;
  }

  if (flags.createFlag()) {
    ScheduleErrorCallback(error_callback, FileError::kSecurityErr);
    return;
  }

  std::unique_ptr<AsyncFileSystemCallbacks> callbacks =
      WTF::MakeUnique<FileSystemEntryCallbacks>(
          success_callback,
          FileSystemEntryScriptErrorCallback::Wrap(error_callback),
          GetExecutionContext(), this, absolute_path, true);
  GetWebFileSystem()->DirectoryExists(CreateFileSystemURL(absolute_path),
                                      std::move(callbacks));
}

int FileSystem::ReadDirectory(
    FileSystemDirectoryReader* reader,
    const String& path,
    FileSystemEntriesCallback* success_callback,
    FileSystemEntryErrorCallbackBase* error_callback) {
  if (!GetExecutionContext())
    return 0;

  if (!GetWebFileSystem()) {
    ScheduleErrorCallback(error_callback, FileError::kAbortErr);
    return 0;
  }

  DCHECK(DOMFilePath::IsAbsolute(path));

  std::unique_ptr<AsyncFileSystemCallbacks> callbacks =
      WTF::MakeUnique<FileSystemEntriesCallbacks>(
          success_callback, error_callback, GetExecutionContext(), reader,
          path);
  return GetWebFileSystem()->ReadDirectory(CreateFileSystemURL(path),
                                           std::move(callbacks));
}

void FileSystem::SnapshotFile(const FileSystemFileEntry* entry,
                              FileCallback* success_callback,
                              ErrorCallback* error_callback) {
  if (!GetExecutionContext())
    return;

  if (!GetWebFileSystem()) {
    ScheduleErrorCallback(error_callback, FileError::kAbortErr);
    return;
  }
  KURL url = CreateFileSystemURL(entry->fullPath());

  std::unique_ptr<AsyncFileSystemCallbacks> callbacks =
      WTF::MakeUnique<SnapshotFileCallbacks>(
          this, entry->name(), url, success_callback,
          FileSystemEntryScriptErrorCallback::Wrap(error_callback),
          GetExecutionContext());
  GetWebFileSystem()->CreateSnapshotFileAndReadMetadata(url,
                                                        std::move(callbacks));
}

KURL FileSystem::CreateFileSystemURL(const String& full_path) const {
  DCHECK(DOMFilePath::IsAbsolute(full_path));
  // For regular types we can just append the entry's fullPath to the
  // root url that should look like 'filesystem:<origin>/<typePrefix>'.
  DCHECK(!filesystem_root_url_.IsEmpty());
  KURL url = filesystem_root_url_;
  // Remove the extra leading slash.
  url.SetPath(url.GetPath() +
              EncodeWithURLEscapeSequences(full_path.Substring(1)));
  return url;
}

bool FileSystem::PathToAbsolutePath(const FileSystemEntry* base,
                                    String path,
                                    String& absolute_path) {
  DCHECK(base);

  if (!DOMFilePath::IsAbsolute(path))
    path = DOMFilePath::Append(base->fullPath(), path);
  absolute_path = DOMFilePath::RemoveExtraParentReferences(path);
  return DOMFilePath::IsValidPath(absolute_path);
}

void FileSystem::Trace(blink::Visitor* visitor) {
  ScriptWrappable::Trace(visitor);
  ContextClient::Trace(visitor);
  visitor->Trace(root_entry_);
}

}  // namespace blink
