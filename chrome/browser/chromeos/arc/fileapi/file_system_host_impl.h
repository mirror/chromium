// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_FILEAPI_FILE_SYSTEM_HOST_IMPL_H_
#define CHROME_BROWSER_CHROMEOS_ARC_FILEAPI_FILE_SYSTEM_HOST_IMPL_H_

#include <stdint.h>

#include <string>

#include "base/macros.h"
#include "components/arc/common/file_system.mojom.h"
#include "storage/browser/fileapi/watcher_manager.h"

namespace arc {

class FileSystemHostImpl : public mojom::FileSystemHost {
 public:
  using ChangeType = storage::WatcherManager::ChangeType;

  class Delegate {
   public:
    virtual ~Delegate() = default;
    virtual void OnDocumentChanged(int64_t watcher_id, ChangeType type) = 0;
  };

  explicit FileSystemHostImpl(Delegate* delegate);
  ~FileSystemHostImpl() override;

  // FileSystemHost overrides:
  void GetFileName(const std::string& url,
                   const GetFileNameCallback& callback) override;
  void GetFileSize(const std::string& url,
                   const GetFileSizeCallback& callback) override;
  void GetFileType(const std::string& url,
                   const GetFileTypeCallback& callback) override;
  void OnDocumentChanged(int64_t watcher_id, ChangeType type) override;
  void OpenFileToRead(const std::string& url,
                      const OpenFileToReadCallback& callback) override;

 private:
  Delegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(FileSystemHostImpl);
};

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_FILEAPI_FILE_SYSTEM_HOST_IMPL_H_
