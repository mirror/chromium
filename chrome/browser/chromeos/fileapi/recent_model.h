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

class Profile;

namespace chromeos {

class RecentContext;
class RecentSource;

// The maximum number of files from single source.
extern const size_t kMaxFilesFromSingleSource;

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

  RecentModel(std::vector<std::unique_ptr<RecentSource>> sources,
              size_t max_files_from_single_source);
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
  struct PerProfileState;
  class FilesMapBuilder;

  void OnFilesMapBuilderDone(std::unique_ptr<FilesMapBuilder> builder,
                             Profile* profile,
                             FilesMap files_map);

  void ClearCache(Profile* profile);

  const std::vector<std::unique_ptr<RecentSource>> sources_;

  // This should be kMaxFilesFromSingleSource except for unit tests.
  const size_t max_files_from_single_source_;

  // Per-profile states. See comments at definitions of PerProfileState.
  std::map<Profile*, PerProfileState> states_;

  base::WeakPtrFactory<RecentModel> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(RecentModel);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_FILEAPI_RECENT_MODEL_H_
