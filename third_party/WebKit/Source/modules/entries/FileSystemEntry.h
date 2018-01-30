// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FileSystemEntry_h
#define FileSystemEntry_h

#include "modules/ModulesExport.h"
#include "modules/entries/FileSystem.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class ErrorCallback;
class FileSystemEntryCallback;

class MODULES_EXPORT FileSystemEntry : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  virtual ~FileSystemEntry();

  virtual bool isFile() const { return false; }
  virtual bool isDirectory() const { return false; }
  const String& name() const { return name_; }
  const String& fullPath() const { return full_path_; }
  FileSystem* filesystem() const { return file_system_; }

  void getParent(FileSystemEntryCallback* success_callback = nullptr,
                 ErrorCallback* = nullptr) const;

  virtual void Trace(blink::Visitor*);

 protected:
  FileSystemEntry(FileSystem*, const String& full_path);

 private:
  Member<FileSystem> file_system_;
  const String full_path_;
  const String name_;
};

}  // namespace blink

#endif  // FileSystemEntry_h
