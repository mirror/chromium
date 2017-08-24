// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/tasks/create_archive_task.h"

#include <memory>

#include "base/bind.h"
#include "base/rand_util.h"
#include "base/time/clock.h"
#include "components/offline_pages/core/archive_manager.h"
#include "components/offline_pages/core/offline_page_model.h"

namespace offline_pages {

namespace {

int64_t GenerateOfflineId() {
  return base::RandGenerator(std::numeric_limits<int64_t>::max()) + 1;
}

}  // namespace

using ArchiverResult = OfflinePageArchiver::ArchiverResult;

CreateArchiveTask::CreateArchiveTask(
    const base::FilePath& archives_dir,
    const OfflinePageModel::SavePageParams& save_page_params,
    std::unique_ptr<OfflinePageArchiver> archiver,
    const CreateArchiveTaskCallback& callback)
    : archives_dir_(archives_dir),
      save_page_params_(save_page_params),
      offline_id_(OfflinePageModel::kInvalidOfflineId),
      archiver_(std::move(archiver)),
      callback_(callback),
      testing_clock_(nullptr),
      skip_clearing_original_url_for_testing_(false),
      weak_ptr_factory_(this) {}

CreateArchiveTask::~CreateArchiveTask() {}

void CreateArchiveTask::Run() {
  CreateArchive();
}

void CreateArchiveTask::CreateArchive() {
  // Skip saving the page that is not intended to be saved, like local file
  // page.
  if (!OfflinePageModel::CanSaveURL(save_page_params_.url)) {
    InformCreationFailed(ArchiverResult::ERROR_SKIPPED);
    return;
  }

  // The web contents is not available if archiver is not created and passed.
  if (!archiver_.get()) {
    InformCreationFailed(ArchiverResult::ERROR_CONTENT_UNAVAILABLE);
    return;
  }

  // If we already have an offline id, use it. If not, generate one.
  offline_id_ = save_page_params_.proposed_offline_id;
  if (offline_id_ == OfflinePageModel::kInvalidOfflineId)
    offline_id_ = GenerateOfflineId();

  OfflinePageArchiver::CreateArchiveParams create_archive_params;
  // If the page is being saved in the background, we should try to remove the
  // popup overlay that obstructs viewing the normal content.
  create_archive_params.remove_popup_overlay = save_page_params_.is_background;
  create_archive_params.use_page_problem_detectors =
      save_page_params_.use_page_problem_detectors;
  archiver_->CreateArchive(
      archives_dir_, create_archive_params,
      base::Bind(&CreateArchiveTask::OnCreateArchiveDone,
                 weak_ptr_factory_.GetWeakPtr(), GetCurrentTime()));
}

void CreateArchiveTask::OnCreateArchiveDone(const base::Time& start_time,
                                            OfflinePageArchiver* archiver,
                                            ArchiverResult archiver_result,
                                            const GURL& saved_url,
                                            const base::FilePath& file_path,
                                            const base::string16& title,
                                            int64_t file_size) {
  DeletePendingArchiver(archiver);
  if (archiver_result != ArchiverResult::SUCCESSFULLY_CREATED) {
    InformCreationFailed(archiver_result);
    return;
  }

  if (save_page_params_.url != saved_url) {
    DVLOG(1) << "Saved URL does not match requested URL.";
    InformCreationFailed(ArchiverResult::ERROR_ARCHIVE_CREATION_FAILED);
    return;
  }

  OfflinePageItem offline_page(saved_url, offline_id_,
                               save_page_params_.client_id, file_path,
                               file_size, start_time);
  offline_page.title = title;
  // Don't record the original URL if it is identical to the final URL. This is
  // because some websites might route the redirect finally back to itself upon
  // the completion of certain action, i.e., authentication, in the middle.
  if (skip_clearing_original_url_for_testing_ ||
      save_page_params_.original_url != offline_page.url) {
    offline_page.original_url = save_page_params_.original_url;
  }
  offline_page.request_origin = save_page_params_.request_origin;

  InformCreationDone(archiver_result, offline_page);
}

void CreateArchiveTask::InformCreationFailed(ArchiverResult result) {
  OfflinePageItem invalid_offline_page;
  InformCreationDone(result, invalid_offline_page);
}

void CreateArchiveTask::InformCreationDone(
    ArchiverResult result,
    const OfflinePageItem& offline_page) {
  callback_.Run(result, offline_page);
  TaskComplete();
}

void CreateArchiveTask::DeletePendingArchiver(OfflinePageArchiver* archiver) {
  DCHECK(archiver == archiver_.get());
  archiver_.reset();
}

base::Time CreateArchiveTask::GetCurrentTime() const {
  return testing_clock_ ? testing_clock_->Now() : base::Time::Now();
}

}  // namespace offline_pages
