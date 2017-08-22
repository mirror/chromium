// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/save_page_task.h"

#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/rand_util.h"
#include "base/time/clock.h"
#include "components/offline_pages/core/archive_manager.h"
#include "components/offline_pages/core/offline_page_archiver.h"
#include "components/offline_pages/core/offline_page_metadata_store.h"
#include "components/offline_pages/core/offline_page_model.h"
#include "components/offline_pages/core/offline_page_model_event_logger.h"

namespace offline_pages {

using ArchiverResult = OfflinePageArchiver::ArchiverResult;

namespace {

SavePageResult ArchiverResultToSavePageResult(ArchiverResult archiver_result) {
  SavePageResult result;
  switch (archiver_result) {
    case ArchiverResult::SUCCESSFULLY_CREATED:
      result = SavePageResult::SUCCESS;
      break;
    case ArchiverResult::ERROR_DEVICE_FULL:
      result = SavePageResult::DEVICE_FULL;
      break;
    case ArchiverResult::ERROR_CONTENT_UNAVAILABLE:
      result = SavePageResult::CONTENT_UNAVAILABLE;
      break;
    case ArchiverResult::ERROR_ARCHIVE_CREATION_FAILED:
      result = SavePageResult::ARCHIVE_CREATION_FAILED;
      break;
    case ArchiverResult::ERROR_CANCELED:
      result = SavePageResult::CANCELLED;
      break;
    case ArchiverResult::ERROR_SECURITY_CERTIFICATE:
      result = SavePageResult::SECURITY_CERTIFICATE_ERROR;
      break;
    case ArchiverResult::ERROR_ERROR_PAGE:
      result = SavePageResult::ERROR_PAGE;
      break;
    case ArchiverResult::ERROR_INTERSTITIAL_PAGE:
      result = SavePageResult::INTERSTITIAL_PAGE;
    default:
      NOTREACHED();
      result = SavePageResult::CONTENT_UNAVAILABLE;
  }
  return result;
}

SavePageResult AddPageResultToSavePageResult(AddPageResult add_result) {
  SavePageResult save_result;
  switch (add_result) {
    case AddPageResult::SUCCESS:
      save_result = SavePageResult::SUCCESS;
      break;
    case AddPageResult::ALREADY_EXISTS:
      save_result = SavePageResult::ALREADY_EXISTS;
      break;
    case AddPageResult::STORE_FAILURE:
      save_result = SavePageResult::STORE_FAILURE;
      break;
    default:
      NOTREACHED();
      save_result = SavePageResult::STORE_FAILURE;
  }
  return save_result;
}

std::string AddHistogramSuffix(const ClientId& client_id,
                               const char* histogram_name) {
  if (client_id.name_space.empty()) {
    NOTREACHED();
    return histogram_name;
  }
  std::string adjusted_histogram_name(histogram_name);
  adjusted_histogram_name += ".";
  adjusted_histogram_name += client_id.name_space;
  return adjusted_histogram_name;
}

void ReportSavePageResultHistogramAfterSave(const ClientId& client_id,
                                            SavePageResult result) {
  // The histogram below is an expansion of the UMA_HISTOGRAM_ENUMERATION
  // macro adapted to allow for a dynamically suffixed histogram name.
  // Note: The factory creates and owns the histogram.
  base::HistogramBase* histogram = base::LinearHistogram::FactoryGet(
      AddHistogramSuffix(client_id, "OfflinePages.SavePageResult"), 1,
      static_cast<int>(SavePageResult::RESULT_COUNT),
      static_cast<int>(SavePageResult::RESULT_COUNT) + 1,
      base::HistogramBase::kUmaTargetedHistogramFlag);
  histogram->Add(static_cast<int>(result));
}

int64_t GenerateOfflineId() {
  return base::RandGenerator(std::numeric_limits<int64_t>::max()) + 1;
}

}  // namespace

SavePageTask::SavePageTask(
    OfflinePageModel* model,
    const OfflinePageModel::SavePageParams& save_page_params,
    std::unique_ptr<OfflinePageArchiver> archiver,
    const SavePageTaskCallback& callback)
    : model_(model),
      save_page_params_(save_page_params),
      callback_(callback),
      archiver_(std::move(archiver)),
      offline_id_(OfflinePageModel::kInvalidOfflineId),
      testing_clock_(nullptr),
      weak_ptr_factory_(this) {}

SavePageTask::~SavePageTask() {}

void SavePageTask::Run() {
  SavePage();
}

void SavePageTask::SavePage() {
  // Skip saving the page that is not intended to be saved, like local file
  // page.
  if (!OfflinePageModel::CanSaveURL(save_page_params_.url)) {
    InformSavePageDone(SavePageResult::SKIPPED);
    return;
  }

  // The web contents is not available if archiver is not created and passed.
  if (!archiver_.get()) {
    InformSavePageDone(SavePageResult::CONTENT_UNAVAILABLE);
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
      model_->GetArchiveManager()->GetArchivesDir(), create_archive_params,
      base::Bind(&SavePageTask::OnCreateArchiveDone,
                 weak_ptr_factory_.GetWeakPtr(), GetCurrentTime()));
}

void SavePageTask::OnCreateArchiveDone(const base::Time& start_time,
                                       OfflinePageArchiver* archiver,
                                       ArchiverResult archiver_result,
                                       const GURL& saved_url,
                                       const base::FilePath& file_path,
                                       const base::string16& title,
                                       int64_t file_size) {
  DeletePendingArchiver(archiver);
  if (archiver_result != ArchiverResult::SUCCESSFULLY_CREATED) {
    SavePageResult result = ArchiverResultToSavePageResult(archiver_result);
    InformSavePageDone(result);
    return;
  }

  if (save_page_params_.url != saved_url) {
    DVLOG(1) << "Saved URL does not match requested URL.";
    InformSavePageDone(SavePageResult::ARCHIVE_CREATION_FAILED);
    return;
  }

  OfflinePageItem offline_page(saved_url, offline_id_,
                               save_page_params_.client_id, file_path,
                               file_size, start_time);
  offline_page.title = title;
  offline_page.original_url = save_page_params_.original_url;
  offline_page.request_origin = save_page_params_.request_origin;

  offline_page_ = offline_page;
  model_->AddPage(offline_page_, base::Bind(&SavePageTask::OnAddOfflinePageDone,
                                            weak_ptr_factory_.GetWeakPtr()));
}

void SavePageTask::OnAddOfflinePageDone(AddPageResult add_result,
                                        int64_t offline_id) {
  InformSavePageDone(AddPageResultToSavePageResult(add_result));
}

void SavePageTask::InformSavePageDone(SavePageResult result) {
  ReportSavePageResultHistogramAfterSave(save_page_params_.client_id, result);
  callback_.Run(result, offline_page_);
  TaskComplete();
}

void SavePageTask::DeletePendingArchiver(OfflinePageArchiver* archiver) {
  DCHECK(archiver == archiver_.get());
  archiver_.reset();
}

base::Time SavePageTask::GetCurrentTime() const {
  return testing_clock_ ? testing_clock_->Now() : base::Time::Now();
}

}  // namespace offline_pages
