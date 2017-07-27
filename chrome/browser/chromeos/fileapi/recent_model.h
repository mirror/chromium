// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_FILEAPI_RECENT_MODEL_H_
#define CHROME_BROWSER_CHROMEOS_FILEAPI_RECENT_MODEL_H_

#include <map>
#include <memory>
#include <vector>

#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "chrome/browser/chromeos/fileapi/recent_file.h"

namespace base {

class TimeTicks;

}  // namespace base

namespace chromeos {

class RecentContext;
class RecentSource;

// Provides the logical list of files in Recent file system.
//
// All member functions must be called on the IO thread.
class RecentModel {
 public:
  // Represents the logical list of files in Recent file system.
  // Key are file names.
  using FilesMap = std::map<base::FilePath, std::unique_ptr<RecentFile>>;
  using GetFilesMapCallback =
      base::OnceCallback<void(const FilesMap& files_map)>;

  explicit RecentModel(std::vector<std::unique_ptr<RecentSource>> sources);
  ~RecentModel();

  // Creates a model with default sources.
  static std::unique_ptr<RecentModel> CreateWithDefaultSources();

  // Returns a list of recent files (as FilesMap) by querying sources.
  // Results are internally cached for better performance. By setting
  // |want_refresh| to true, you can request the model to refresh the cache
  // (but it is not always honored; we just use shorter expiration for now).
  void GetFilesMap(RecentContext context,
                   bool want_refresh,
                   GetFilesMapCallback callback);

 private:
  class FilesMapBuilder;

  void OnFilesMapBuilderDone(std::unique_ptr<FilesMapBuilder> builder,
                             base::TimeTicks build_time,
                             FilesMap files_map);

  const std::vector<std::unique_ptr<RecentSource>> sources_;

  // Cached FilesMap.
  FilesMap files_map_cache_;

  // Time when |files_map_cache_| was built.
  base::TimeTicks files_map_cache_time_;

  // While a FilesMap is built, this vector contains callbacks to be invoked
  // with the new FilesMap.
  std::vector<GetFilesMapCallback> pending_callbacks_;

  base::WeakPtrFactory<RecentModel> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(RecentModel);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_FILEAPI_RECENT_MODEL_H_
