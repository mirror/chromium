// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DEVTOOLS_DEVTOOLS_FILE_WATCHER_H_
#define CHROME_BROWSER_DEVTOOLS_DEVTOOLS_FILE_WATCHER_H_

#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/sequence_checker.h"

namespace base {
class FilePath;
class SequencedTaskRunner;
}

class DevToolsFileWatcher {
 public:
  using WatchCallback = base::Callback<void(const std::vector<std::string>&,
                                            const std::vector<std::string>&,
                                            const std::vector<std::string>&)>;
  explicit DevToolsFileWatcher(const WatchCallback& callback);
  ~DevToolsFileWatcher();

  void AddWatch(const base::FilePath& path);
  void RemoveWatch(const base::FilePath& path);

  static base::SequencedTaskRunner* impl_task_runner();

 private:
  class SharedFileWatcher;
  static SharedFileWatcher* s_shared_watcher_;

  void InitSharedWatcher();
  void FileChanged(const base::FilePath&, int);

  scoped_refptr<SharedFileWatcher> shared_watcher_;
  WatchCallback callback_;
  scoped_refptr<base::SequencedTaskRunner> client_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsFileWatcher);
};

#endif  // CHROME_BROWSER_DEVTOOLS_DEVTOOLS_FILE_WATCHER_H_
