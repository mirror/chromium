// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/fileapi/recent_model.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "chrome/browser/chromeos/fileapi/recent_arc_media_source.h"
#include "chrome/browser/chromeos/fileapi/recent_context.h"
#include "chrome/browser/chromeos/fileapi/recent_source.h"
#include "content/public/browser/browser_thread.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/fileapi/file_system_operation.h"

using content::BrowserThread;

namespace chromeos {

namespace {

// Any queries will never cause files map cache to be updated for this duration.
constexpr base::TimeDelta kHardCacheExpiration =
    base::TimeDelta::FromSeconds(3);

// Queries with want_refresh=false will not cause files map cache to be updated
// for this duration.
constexpr base::TimeDelta kSoftCacheExpiration =
    base::TimeDelta::FromSeconds(60);

struct RecentFileComparator {
  bool operator()(const std::unique_ptr<RecentFile>& a,
                  const std::unique_ptr<RecentFile>& b) {
    if (a->source_type() != b->source_type())
      return a->source_type() < b->source_type();
    if (a->unique_id() != b->unique_id())
      return a->unique_id() < b->unique_id();
    return a->original_file_name() < b->original_file_name();
  }
};

}  // namespace

// A tuple of values that identifies the file system operation requester.
// This is used, for example, as a key to cache file entries.
struct RecentModel::Identity {
  Profile* profile;
  GURL origin;

  static Identity FromRecentContext(const RecentContext& context) {
    return Identity{context.profile(), context.origin()};
  }

  friend bool operator<(const Identity& a, const Identity& b) {
    if (a.profile != b.profile)
      return a.profile < b.profile;
    return a.origin < b.origin;
  }
};

// Per-identity state.
struct RecentModel::State {
  // Cached FilesMap.
  FilesMap files_map_cache;

  // Time when |files_map_cache_| was built.
  base::TimeTicks files_map_cache_time;

  // While a FilesMap is built, this vector contains callbacks to be invoked
  // with the new FilesMap.
  std::vector<GetFilesMapCallback> pending_callbacks;
};

class RecentModel::FilesMapBuilder {
 public:
  using BuildFilesMapCallback = base::OnceCallback<void(FilesMap files_map)>;

  FilesMapBuilder();
  ~FilesMapBuilder();

  void Build(const std::vector<std::unique_ptr<RecentSource>>& sources,
             RecentContext context,
             BuildFilesMapCallback callback);

 private:
  void OnGetRecentFiles(std::vector<std::unique_ptr<RecentFile>> files);
  void OnGetRecentFilesCompleted();

  int num_inflight_sources_;
  BuildFilesMapCallback callback_;

  std::vector<std::unique_ptr<RecentFile>> files_;

  DISALLOW_COPY_AND_ASSIGN(FilesMapBuilder);
};

RecentModel::FilesMapBuilder::FilesMapBuilder() : num_inflight_sources_(-1) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

RecentModel::FilesMapBuilder::~FilesMapBuilder() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

void RecentModel::FilesMapBuilder::Build(
    const std::vector<std::unique_ptr<RecentSource>>& sources,
    RecentContext context,
    BuildFilesMapCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK_LT(num_inflight_sources_, 0);

  num_inflight_sources_ = sources.size();
  callback_ = std::move(callback);

  if (sources.empty()) {
    OnGetRecentFilesCompleted();
    return;
  }

  for (const auto& source : sources) {
    source->GetRecentFiles(context,
                           base::BindOnce(&FilesMapBuilder::OnGetRecentFiles,
                                          base::Unretained(this)));
  }
}

void RecentModel::FilesMapBuilder::OnGetRecentFiles(
    std::vector<std::unique_ptr<RecentFile>> files) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  files_.insert(files_.end(), std::make_move_iterator(files.begin()),
                std::make_move_iterator(files.end()));

  --num_inflight_sources_;
  if (num_inflight_sources_ == 0)
    OnGetRecentFilesCompleted();
}

void RecentModel::FilesMapBuilder::OnGetRecentFilesCompleted() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  // Sort files to keep the list stable as far as possible.
  std::sort(files_.begin(), files_.end(), RecentFileComparator());

  FilesMap files_map;
  std::map<base::FilePath, int> suffix_counters;

  for (std::unique_ptr<RecentFile>& file : files_) {
    base::FilePath filename(file->original_file_name());

    if (files_map.count(filename) > 0) {
      // Resolve a conflict by adding a suffix.
      int& suffix_counter = suffix_counters[filename];
      while (true) {
        ++suffix_counter;
        std::string suffix = base::StringPrintf(" (%d)", suffix_counter);
        base::FilePath new_filename =
            base::FilePath(filename).InsertBeforeExtensionASCII(suffix);
        if (files_map.count(new_filename) == 0) {
          filename = new_filename;
          break;
        }
      }
    }

    DCHECK_EQ(0u, files_map.count(filename));
    files_map.insert(std::make_pair(filename, std::move(file)));
  }

  std::move(callback_).Run(std::move(files_map));
}

RecentModel::RecentModel(std::vector<std::unique_ptr<RecentSource>> sources)
    : sources_(std::move(sources)), weak_ptr_factory_(this) {}

RecentModel::~RecentModel() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

// static
std::unique_ptr<RecentModel> RecentModel::CreateWithDefaultSources() {
  std::vector<std::unique_ptr<RecentSource>> sources;
  sources.emplace_back(base::MakeUnique<RecentArcMediaSource>());
  return base::MakeUnique<RecentModel>(std::move(sources));
}

void RecentModel::GetFilesMap(RecentContext context,
                              bool want_refresh,
                              GetFilesMapCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  Identity identity = Identity::FromRecentContext(context);
  State& state = states_[identity];

  base::TimeTicks current_time = base::TimeTicks::Now();
  base::TimeDelta expiration =
      want_refresh ? kHardCacheExpiration : kSoftCacheExpiration;
  if (!state.files_map_cache_time.is_null() &&
      current_time - state.files_map_cache_time < expiration) {
    // Use cache.
    std::move(callback).Run(state.files_map_cache);
    return;
  }

  bool builder_already_running = !state.pending_callbacks.empty();
  state.pending_callbacks.emplace_back(std::move(callback));

  // If a builder is already running, just enqueue the callback and return.
  if (builder_already_running)
    return;

  std::unique_ptr<FilesMapBuilder> builder =
      base::MakeUnique<FilesMapBuilder>();
  FilesMapBuilder* builder_ptr = builder.get();
  builder_ptr->Build(
      sources_, std::move(context),
      base::BindOnce(&RecentModel::OnFilesMapBuilderDone,
                     weak_ptr_factory_.GetWeakPtr(),
                     base::Passed(std::move(builder)), identity, current_time));
}

void RecentModel::OnFilesMapBuilderDone(
    std::unique_ptr<FilesMapBuilder> builder,
    const Identity& identity,
    base::TimeTicks build_time,
    FilesMap files_map) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  State& state = states_[identity];

  state.files_map_cache = std::move(files_map);
  state.files_map_cache_time = build_time;

  std::vector<GetFilesMapCallback> pending_callbacks;
  pending_callbacks.swap(state.pending_callbacks);
  DCHECK(!pending_callbacks.empty());
  for (auto& callback : pending_callbacks) {
    std::move(callback).Run(state.files_map_cache);
  }
}

}  // namespace chromeos
