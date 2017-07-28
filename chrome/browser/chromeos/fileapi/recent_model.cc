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
#include "base/timer/timer.h"
#include "chrome/browser/chromeos/fileapi/recent_context.h"
#include "chrome/browser/chromeos/fileapi/recent_source.h"
#include "content/public/browser/browser_thread.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/fileapi/file_system_operation.h"

using content::BrowserThread;

namespace chromeos {

namespace {

// FilesMap cache will be cleared this duration after it is built.
constexpr base::TimeDelta kHardCacheExpiration =
    base::TimeDelta::FromSeconds(60);

// FilesMap cache will be refreshed this duration after it is built
// if GetFilesMap() is called with |want_refresh| = true.
constexpr base::TimeDelta kSoftCacheExpiration =
    base::TimeDelta::FromSeconds(3);

struct StableComparator {
  bool operator()(const std::unique_ptr<RecentFile>& a,
                  const std::unique_ptr<RecentFile>& b) {
    if (a->source_type() != b->source_type())
      return a->source_type() < b->source_type();
    if (a->unique_id() != b->unique_id())
      return a->unique_id() < b->unique_id();
    return a->file_name() < b->file_name();
  }
};

struct LastModifiedTimeComparator {
  bool operator()(const std::unique_ptr<RecentFile>& a,
                  const std::unique_ptr<RecentFile>& b) {
    // Most recently modified file will come first.
    if (a->file_info().last_modified != b->file_info().last_modified)
      return a->file_info().last_modified > b->file_info().last_modified;
    // Fallback to StableComparator.
    return StableComparator()(a, b);
  }
};

}  // namespace

const size_t kMaxFilesFromSingleSource = 1000;

// Per-profile state.
struct RecentModel::PerProfileState {
  // Cached FilesMap.
  FilesMap files_map_cache;

  // Time when |files_map_cache_| was built.
  base::TimeTicks files_map_cache_time;

  // Timer to clear the cache.
  base::OneShotTimer cache_clear_timer;

  // While a FilesMap is built, this vector contains callbacks to be invoked
  // with the new FilesMap.
  std::vector<GetFilesMapCallback> pending_callbacks;
};

class RecentModel::FilesMapBuilder {
 public:
  using BuildFilesMapCallback = base::OnceCallback<void(FilesMap files_map)>;

  explicit FilesMapBuilder(size_t max_files_from_single_source);
  ~FilesMapBuilder();

  void Build(const std::vector<std::unique_ptr<RecentSource>>& sources,
             RecentContext context,
             BuildFilesMapCallback callback);

 private:
  void OnGetRecentFiles(std::vector<std::unique_ptr<RecentFile>> files);
  void OnGetRecentFilesCompleted();

  // This should be kMaxFilesFromSingleSource except for unit tests.
  const size_t max_files_from_single_source_;

  int num_inflight_sources_;
  BuildFilesMapCallback callback_;

  std::vector<std::unique_ptr<RecentFile>> files_;

  DISALLOW_COPY_AND_ASSIGN(FilesMapBuilder);
};

RecentModel::FilesMapBuilder::FilesMapBuilder(
    size_t max_files_from_single_source)
    : max_files_from_single_source_(max_files_from_single_source),
      num_inflight_sources_(-1) {
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

  // Cap the number of files per source.
  if (files.size() > max_files_from_single_source_) {
    std::sort(files.begin(), files.end(), LastModifiedTimeComparator());
    files.erase(files.begin() + max_files_from_single_source_, files.end());
  }

  files_.insert(files_.end(), std::make_move_iterator(files.begin()),
                std::make_move_iterator(files.end()));

  --num_inflight_sources_;
  if (num_inflight_sources_ == 0)
    OnGetRecentFilesCompleted();
}

void RecentModel::FilesMapBuilder::OnGetRecentFilesCompleted() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  // Sort files to keep the list stable as far as possible.
  std::sort(files_.begin(), files_.end(), StableComparator());

  FilesMap files_map;
  std::map<base::FilePath, int> suffix_counters;

  for (std::unique_ptr<RecentFile>& file : files_) {
    base::FilePath file_name(file->file_name());

    if (files_map.count(file_name) > 0) {
      // Resolve a conflict by adding a suffix.
      int& suffix_counter = suffix_counters[file_name];
      while (true) {
        ++suffix_counter;
        std::string suffix = base::StringPrintf(" (%d)", suffix_counter);
        base::FilePath new_file_name =
            base::FilePath(file_name).InsertBeforeExtensionASCII(suffix);
        if (files_map.count(new_file_name) == 0) {
          file_name = new_file_name;
          break;
        }
      }
    }

    DCHECK_EQ(0u, files_map.count(file_name));
    files_map.insert(std::make_pair(file_name, std::move(file)));
  }

  std::move(callback_).Run(std::move(files_map));
}

RecentModel::RecentModel(std::vector<std::unique_ptr<RecentSource>> sources,
                         const size_t max_files_from_single_source)
    : sources_(std::move(sources)),
      max_files_from_single_source_(max_files_from_single_source),
      weak_ptr_factory_(this) {}

RecentModel::~RecentModel() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

// static
std::unique_ptr<RecentModel> RecentModel::CreateWithDefaultSources() {
  std::vector<std::unique_ptr<RecentSource>> sources;
  // TODO(nya): Add source implementations.
  return base::MakeUnique<RecentModel>(std::move(sources),
                                       kMaxFilesFromSingleSource);
}

void RecentModel::GetFilesMap(RecentContext context,
                              bool want_refresh,
                              GetFilesMapCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  PerProfileState& state = states_[context.profile()];

  base::TimeTicks current_time = base::TimeTicks::Now();
  base::TimeDelta expiration =
      want_refresh ? kSoftCacheExpiration : kHardCacheExpiration;
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
      base::MakeUnique<FilesMapBuilder>(max_files_from_single_source_);
  FilesMapBuilder* builder_ptr = builder.get();
  builder_ptr->Build(sources_, std::move(context),
                     base::BindOnce(&RecentModel::OnFilesMapBuilderDone,
                                    weak_ptr_factory_.GetWeakPtr(),
                                    base::Passed(std::move(builder)),
                                    base::Unretained(context.profile())));
}

void RecentModel::OnFilesMapBuilderDone(
    std::unique_ptr<FilesMapBuilder> builder,
    Profile* profile,
    FilesMap files_map) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  PerProfileState& state = states_[profile];

  state.files_map_cache = std::move(files_map);
  state.files_map_cache_time = base::TimeTicks::Now();
  state.cache_clear_timer.Start(
      FROM_HERE, kHardCacheExpiration,
      base::Bind(&RecentModel::ClearCache, weak_ptr_factory_.GetWeakPtr(),
                 base::Unretained(profile)));

  std::vector<GetFilesMapCallback> pending_callbacks;
  pending_callbacks.swap(state.pending_callbacks);
  DCHECK(!pending_callbacks.empty());
  for (auto& callback : pending_callbacks)
    std::move(callback).Run(state.files_map_cache);
}

void RecentModel::ClearCache(Profile* profile) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  PerProfileState& state = states_[profile];

  if (state.pending_callbacks.empty()) {
    // If there is no pending callback, it is safe to delete State.
    states_.erase(profile);
    return;
  }

  // Since there are pending callbacks, we can't delete State.
  // We just clear cache now, and once the current build finishes, a new
  // timer is started and ClearCache() will be called again.
  state.files_map_cache.clear();
  state.files_map_cache_time = base::TimeTicks();
}

}  // namespace chromeos
