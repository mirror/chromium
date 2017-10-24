// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FileSystemDirectoryEntry_h
#define FileSystemDirectoryEntry_h

#include "modules/ModulesExport.h"
#include "modules/entries/FileSystemEntry.h"
#include "modules/filesystem/FileSystemFlags.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class ErrorCallback;
class FileSystem;
class FileSystemDirectoryReader;
class FileSystemEntryCallback;

class MODULES_EXPORT FileSystemDirectoryEntry final : public FileSystemEntry {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static FileSystemDirectoryEntry* Create(FileSystem* file_system,
                                          const String& full_path) {
    return new FileSystemDirectoryEntry(file_system, full_path);
  }
  bool isDirectory() const override { return true; }

  FileSystemDirectoryReader* createReader();
  void getFile(const String& path = String(),
               const FileSystemFlags& = FileSystemFlags(),
               FileSystemEntryCallback* = nullptr,
               ErrorCallback* = nullptr);
  void getDirectory(const String& path = String(),
                    const FileSystemFlags& = FileSystemFlags(),
                    FileSystemEntryCallback* = nullptr,
                    ErrorCallback* = nullptr);

  virtual void Trace(blink::Visitor*);

 private:
  FileSystemDirectoryEntry(FileSystem*, const String& full_path);
};

DEFINE_TYPE_CASTS(FileSystemDirectoryEntry,
                  FileSystemEntry,
                  entry,
                  entry->isDirectory(),
                  entry.isDirectory());

}  // namespace blink

#endif  // FileSystemDirectoryEntry_h
