// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_IMAGE_RETAINER_H_
#define CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_IMAGE_RETAINER_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"

class GURL;

namespace base {
class FilePath;
class SequencedTaskRunner;
}  // namespace base

namespace gfx {
class Image;
}

// Interface for the Temp File Keeper class.
class NotificationImageRetainer {
 public:
  virtual ~NotificationImageRetainer() {}

  virtual base::FilePath RegisterTemporaryImage(gfx::Image image,
                                                const std::string& profile_id,
                                                const GURL& origin) = 0;
};

// This purpose of this class is to take data from memory, store it to disk as
// temp files and keep them alive long enough to hand over to external entities,
// such as the Action Center on Windows.
//
// The reason for its existence is that on Windows, temp file deletion is not
// guaranteed and, since the images can potentially be large, this presents a
// problem because Chrome might then be leaving chunks of dead bits lying around
// on user’s computers during unclean shutdowns.
class NotificationImageRetainerImpl : public NotificationImageRetainer {
 public:
  NotificationImageRetainerImpl(const base::string16& dir_prefix,
                                base::SequencedTaskRunner* task_runner);
  ~NotificationImageRetainerImpl() override;

  // Stores an |image| from a particular profile (|profile_id|) and |origin| on
  // disk in a temporary (short-lived) file. Returns the path to the file
  // created, which will be valid for a few seconds only. It will be deleted
  // either after a short timeout or after a restart of Chrome (the next time
  // this function is called). The function returns an empty FilePath if file
  // creation fails.
  base::FilePath RegisterTemporaryImage(gfx::Image image,
                                        const std::string& profile_id,
                                        const GURL& origin) override;

 private:
  // Performs a one-time reset of the temp file structure (flushes out old
  // orphaned files and starts a new temp directory).
  void ResetTempFiles();

  // Deletes orphaned files left from previous runs.
  void DeleteOrphanedImages();

  // Returns the temporary directory within the user data directory.
  base::FilePath GetBaseTempDirectory();

  // The path to where to store the temporary files.
  base::FilePath temp_path_;

  // The prefix for the temporary directory to use.
  base::string16 dir_prefix_;

  // Whether a one-time cleanup of old files has been performed.
  bool cleanup_performed_;

  // The task runner to use. Weak, not owned by us.
  base::SequencedTaskRunner* task_runner_;

  DISALLOW_COPY_AND_ASSIGN(NotificationImageRetainerImpl);
};

#endif  // CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_IMAGE_RETAINER_H_
