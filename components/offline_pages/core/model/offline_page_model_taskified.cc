// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/offline_page_model_taskified.h"

#include <memory>
#include <vector>

#include "base/bind.h"
#include "base/location.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string16.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "components/offline_pages/core/archive_manager.h"
#include "components/offline_pages/core/model/add_page_task.h"
#include "components/offline_pages/core/model/create_archive_task.h"
#include "components/offline_pages/core/model/delete_page_task.h"
#include "components/offline_pages/core/model/mark_page_accessed_task.h"
#include "components/offline_pages/core/offline_page_metadata_store.h"
#include "components/offline_pages/core/offline_page_metadata_store_sql.h"
#include "components/offline_pages/core/offline_page_model.h"
#include "url/gurl.h"

namespace offline_pages {

using ArchiverResult = OfflinePageArchiver::ArchiverResult;
using ClearStorageResult = ClearStorageTask::ClearStorageResult;

namespace {

const base::TimeDelta kClearStorageStartingDelay =
    base::TimeDelta::FromSeconds(20);

SavePageResult ToSavePageResult(ArchiverResult archiver_result) {
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
      break;
    case ArchiverResult::ERROR_SKIPPED:
      result = SavePageResult::SKIPPED;
      break;
    default:
      NOTREACHED();
      result = SavePageResult::CONTENT_UNAVAILABLE;
  }
  return result;
}

}  // namespace

OfflinePageModelTaskified::OfflinePageModelTaskified(
    std::unique_ptr<OfflinePageMetadataStoreSQL> store,
    std::unique_ptr<ArchiveManager> archive_manager,
    const scoped_refptr<base::SequencedTaskRunner>& task_runner)
    : task_queue_(this),
      store_(std::move(store)),
      archive_manager_(std::move(archive_manager)),
      policy_controller_(new ClientPolicyController()),
      weak_ptr_factory_(this) {
  // No callback is required here.
  archive_manager_->EnsureArchivesDirCreated(base::Bind([]() {}));
}

OfflinePageModelTaskified::~OfflinePageModelTaskified() {}

void OfflinePageModelTaskified::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void OfflinePageModelTaskified::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void OfflinePageModelTaskified::OnTaskQueueIsIdle() {}

void OfflinePageModelTaskified::SavePage(
    const SavePageParams& save_page_params,
    std::unique_ptr<OfflinePageArchiver> archiver,
    const SavePageCallback& callback) {
  auto task = base::MakeUnique<CreateArchiveTask>(
      GetArchiveDirectory(save_page_params.client_id.name_space),
      save_page_params, archiver.get(),
      base::Bind(&OfflinePageModelTaskified::OnCreateArchiveDone,
                 weak_ptr_factory_.GetWeakPtr(), callback));
  DCHECK(task);
  pending_archivers_.push_back(std::move(archiver));
  task_queue_.AddTask(std::move(task));
}

void OfflinePageModelTaskified::AddPage(const OfflinePageItem& page,
                                        const AddPageCallback& callback) {
  auto task = base::MakeUnique<AddPageTask>(
      store_.get(), page,
      base::BindOnce(&OfflinePageModelTaskified::OnAddPageDone,
                     weak_ptr_factory_.GetWeakPtr(), page, callback));
  DCHECK(task);
  task_queue_.AddTask(std::move(task));
}

void OfflinePageModelTaskified::MarkPageAccessed(int64_t offline_id) {
  auto task = base::MakeUnique<MarkPageAccessedTask>(store_.get(), offline_id,
                                                     base::Time::Now());
  DCHECK(task);
  task_queue_.AddTask(std::move(task));
}

void OfflinePageModelTaskified::DeletePagesByOfflineId(
    const std::vector<int64_t>& offline_ids,
    const DeletePageCallback& callback) {}

void OfflinePageModelTaskified::DeletePagesByClientIds(
    const std::vector<ClientId>& client_ids,
    const DeletePageCallback& callback) {}

void OfflinePageModelTaskified::DeleteCachedPagesByURLPredicate(
    const UrlPredicate& predicate,
    const DeletePageCallback& callback) {}

void OfflinePageModelTaskified::GetAllPages(
    const MultipleOfflinePageItemCallback& callback) {}

void OfflinePageModelTaskified::GetPageByOfflineId(
    int64_t offline_id,
    const SingleOfflinePageItemCallback& callback) {}

void OfflinePageModelTaskified::GetPagesByClientIds(
    const std::vector<ClientId>& client_ids,
    const MultipleOfflinePageItemCallback& callback) {}

void OfflinePageModelTaskified::GetPagesByURL(
    const GURL& url,
    URLSearchMode url_search_mode,
    const MultipleOfflinePageItemCallback& callback) {}

void OfflinePageModelTaskified::GetPagesByNamespace(
    const std::string& name_space,
    const MultipleOfflinePageItemCallback& callback) {}

void OfflinePageModelTaskified::GetPagesRemovedOnCacheReset(
    const MultipleOfflinePageItemCallback& callback) {}

void OfflinePageModelTaskified::GetPagesSupportedByDownloads(
    const MultipleOfflinePageItemCallback& callback) {}

void OfflinePageModelTaskified::GetPagesByRequestOrigin(
    const std::string& request_origin,
    const MultipleOfflinePageItemCallback& callback) {}

void OfflinePageModelTaskified::GetOfflineIdsForClientId(
    const ClientId& client_id,
    const MultipleOfflineIdCallback& callback) {}

const base::FilePath& OfflinePageModelTaskified::GetArchiveDirectory(
    const std::string& name_space) const {
  if (policy_controller_->IsRemovedOnCacheReset(name_space))
    return archive_manager_->GetTemporaryArchivesDir();
  return archive_manager_->GetPersistentArchivesDir();
}

void OfflinePageModelTaskified::CheckPagesExistOffline(
    const std::set<GURL>& urls,
    const CheckPagesExistOfflineCallback& callback) {
  // TODO(romax): Remove the method after switch.
  NOTIMPLEMENTED();
}

bool OfflinePageModelTaskified::is_loaded() const {
  NOTIMPLEMENTED();
  // TODO(romax): Remove the method after switch. No longer needed with
  // DB.Execute pattern.
  return false;
}

ClientPolicyController* OfflinePageModelTaskified::GetPolicyController() {
  return policy_controller_.get();
}

OfflineEventLogger* OfflinePageModelTaskified::GetLogger() {
  return &offline_event_logger_;
}

void OfflinePageModelTaskified::InformSavePageDone(
    const SavePageCallback& callback,
    SavePageResult result,
    const OfflinePageItem& page) {
  if (result == SavePageResult::ARCHIVE_CREATION_FAILED)
    archive_manager_->EnsureArchivesDirCreated(base::Bind([]() {}));
  if (!callback.is_null())
    callback.Run(result, page.offline_id);
}

void OfflinePageModelTaskified::OnAddPageForSavePageDone(
    const SavePageCallback& callback,
    const OfflinePageItem& proposed_page,
    AddPageResult add_page_result,
    int64_t offline_id) {
  SavePageResult save_page_result;
  if (add_page_result == AddPageResult::SUCCESS) {
    save_page_result = SavePageResult::SUCCESS;
  } else if (add_page_result == AddPageResult::ALREADY_EXISTS) {
    save_page_result = SavePageResult::ALREADY_EXISTS;
  } else if (add_page_result == AddPageResult::STORE_FAILURE) {
    save_page_result = SavePageResult::STORE_FAILURE;
  } else {
    NOTREACHED();
    save_page_result = SavePageResult::STORE_FAILURE;
  }
  InformSavePageDone(callback, save_page_result, proposed_page);
  if (save_page_result == SavePageResult::SUCCESS) {
    RemovePagesWithSameUrlInSameNamespace(proposed_page);
  }
  PostClearCachedPagesTask(false /* is_delayed */);
}

void OfflinePageModelTaskified::OnCreateArchiveDone(
    const SavePageCallback& callback,
    OfflinePageItem proposed_page,
    OfflinePageArchiver* archiver,
    ArchiverResult archiver_result,
    const GURL& saved_url,
    const base::FilePath& file_path,
    const base::string16& title,
    int64_t file_size) {
  pending_archivers_.erase(
      std::find_if(pending_archivers_.begin(), pending_archivers_.end(),
                   [archiver](const std::unique_ptr<OfflinePageArchiver>& a) {
                     return a.get() == archiver;
                   }));

  if (archiver_result != ArchiverResult::SUCCESSFULLY_CREATED) {
    SavePageResult result = ToSavePageResult(archiver_result);
    InformSavePageDone(callback, result, proposed_page);
    return;
  }
  if (proposed_page.url != saved_url) {
    DVLOG(1) << "Saved URL does not match requested URL.";
    InformSavePageDone(callback, SavePageResult::ARCHIVE_CREATION_FAILED,
                       proposed_page);
    return;
  }
  proposed_page.file_path = file_path;
  proposed_page.title = title;
  proposed_page.file_size = file_size;
  AddPage(proposed_page,
          base::Bind(&OfflinePageModelTaskified::OnAddPageForSavePageDone,
                     weak_ptr_factory_.GetWeakPtr(), callback, proposed_page));
}

void OfflinePageModelTaskified::OnAddPageDone(const OfflinePageItem& page,
                                              const AddPageCallback& callback,
                                              AddPageResult result) {
  callback.Run(result, page.offline_id);
  if (result == AddPageResult::SUCCESS) {
    for (Observer& observer : observers_)
      observer.OfflinePageAdded(this, page);
  }
}

void OfflinePageModelTaskified::PostClearCachedPagesTask(bool is_delayed) {
  base::TimeDelta delay =
      is_delayed ? kClearStorageStartingDelay : base::TimeDelta();
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&OfflinePageModelTaskified::ClearCachedPages,
                 weak_ptr_factory_.GetWeakPtr()),
      delay);
}

void OfflinePageModelTaskified::InformDeletePageDone(
    const DeletePageCallback& callback,
    DeletePageResult result) {
  if (!callback.is_null())
    callback.Run(result);
}

void OfflinePageModelTaskified::OnDeleteDone(
    const DeletePageCallback& callback,
    DeletePageResult result,
    const std::vector<OfflinePageModel::DeletedPageInfo>& infos) {
  for (const auto& info : infos)
    for (Observer& observer : observers_)
      observer.OfflinePageDeleted(info);
  InformDeletePageDone(callback, result);
}

void OfflinePageModelTaskified::ClearCachedPages() {
  auto task = base::MakeUnique<ClearStorageTask>(
      store_.get(), archive_manager_.get(), policy_controller_.get(),
      last_clear_cached_pages_time_, base::Time::Now(),
      base::BindOnce(&OfflinePageModelTaskified::OnClearCachedPagesDone,
                     weak_ptr_factory_.GetWeakPtr(), base::Time::Now()));
  DCHECK(task);
  task_queue_.AddTask(std::move(task));
}

void OfflinePageModelTaskified::OnClearCachedPagesDone(
    base::Time start_time,
    size_t deleted_page_count,
    ClearStorageResult result) {
  last_clear_cached_pages_time_ = start_time;
}

void OfflinePageModelTaskified::RemovePagesWithSameUrlInSameNamespace(
    const OfflinePageItem& page) {
  auto task = DeletePageTask::CreateTaskDeletingForPageLimit(
      store_.get(),
      base::BindOnce(&OfflinePageModelTaskified::OnDeleteDone,
                     weak_ptr_factory_.GetWeakPtr(),
                     base::Bind([](DeletePageResult result) {})),
      policy_controller_.get(), page);
  if (task)
    task_queue_.AddTask(std::move(task));
}

}  // namespace offline_pages
