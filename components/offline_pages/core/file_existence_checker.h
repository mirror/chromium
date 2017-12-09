// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_FILE_EXISTENCE_CHECKER_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_FILE_EXISTENCE_CHECKER_H_

#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/task_runner_util.h"

namespace base {
class FilePath;
class SequencedTaskRunner;
}  // namespace base

namespace offline_pages {

// Class that checks in bulk, which of the provided set of file paths are not
// pointing to an existing file.
class FileExistenceChecker {
 public:
  // Callback through which the result (items detected to be missing) is passed
  // back.
  typedef base::OnceCallback<void(
      std::vector<base::FilePath> /* missing items */)>
      ResultCallback;

  explicit FileExistenceChecker(
      const scoped_refptr<base::SequencedTaskRunner>& blocking_task_runner);
  ~FileExistenceChecker();

  // Checks which of the provided |files_paths| don't point to existing files.
  void CheckForMissingFiles(std::vector<base::FilePath> files_paths,
                            ResultCallback callback);

 private:
  // Blocking task runner, which will be used to check files.
  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(FileExistenceChecker);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_FILE_EXISTENCE_CHECKER_H_
