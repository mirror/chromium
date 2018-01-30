// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FileSystemFileEntry_h
#define FileSystemFileEntry_h

#include "modules/ModulesExport.h"
#include "modules/entries/FileSystemEntry.h"
#include "platform/heap/Handle.h"

namespace blink {

class FileSystem;
class FileCallback;

// Implementation of FileSystemFileEntry interface.

class MODULES_EXPORT FileSystemFileEntry final : public FileSystemEntry {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static FileSystemFileEntry* Create(FileSystem* file_system,
                                     const String& full_path) {
    return new FileSystemFileEntry(file_system, full_path);
  }

  // FileSystemFileEntry.idl:
  void file(FileCallback*, ErrorCallback* = nullptr) const;

  bool isFile() const override { return true; }

  virtual void Trace(blink::Visitor*);

 private:
  FileSystemFileEntry(FileSystem*, const String& full_path);
};

DEFINE_TYPE_CASTS(FileSystemFileEntry,
                  FileSystemEntry,
                  entry,
                  entry->isFile(),
                  entry.isFile());

}  // namespace blink

#endif  // FileSystemFileEntry_h
