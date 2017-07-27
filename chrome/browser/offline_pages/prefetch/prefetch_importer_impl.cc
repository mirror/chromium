// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_pages/prefetch/prefetch_importer_impl.h"

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/guid.h"
#include "base/logging.h"
#include "base/task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "chrome/browser/offline_pages/offline_page_model_factory.h"
#include "chrome/common/chrome_constants.h"
#include "components/offline_pages/core/offline_page_model.h"
#include "components/offline_pages/core/prefetch/prefetch_item.h"
#include "content/public/browser/browser_context.h"
#include "url/gurl.h"

namespace offline_pages {

namespace {

const base::FilePath::CharType kMHTMLExtension[] = FILE_PATH_LITERAL("mhtml");

void MoveFile(const base::FilePath& src_path,
              const base::FilePath& dest_path,
              base::TaskRunner* task_runner,
              const base::Callback<void(bool)>& callback) {
  bool success = base::Move(src_path, dest_path);
  task_runner->PostTask(FROM_HERE, base::Bind(callback, success));
}

}  // namespace

PrefetchImporterImpl::PrefetchImporterImpl(
    content::BrowserContext* context,
    scoped_refptr<base::TaskRunner> background_task_runner)
    : context_(context),
      background_task_runner_(background_task_runner),
      weak_ptr_factory_(this) {}

PrefetchImporterImpl::~PrefetchImporterImpl() = default;

void PrefetchImporterImpl::ImportArchive(const ArchiveInfo& params,
                                         const CompletedCallback& callback) {
  // The target file name will be auto generated based on GUID to prevent any
  // name collision.
  base::FilePath archives_dir =
      context_->GetPath().Append(chrome::kOfflinePageArchivesDirname);
  base::FilePath dest_path =
      archives_dir.Append(base::GenerateGUID()).AddExtension(kMHTMLExtension);

  GURL url, original_url;
  if (params.url != params.final_archived_url) {
    url = params.final_archived_url;
    original_url = params.url;
  } else {
    url = params.url;
    original_url = params.url;
  }
  OfflinePageItem offline_page(url, params.offline_id, params.client_id,
                               dest_path, params.file_size, base::Time::Now());
  offline_page.original_url = original_url;
  offline_page.title = params.title;

  // Moves the file from download directory to offline archive directory. The
  // file move operation should be done on background thread.
  background_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(
          &MoveFile, params.file_path, dest_path,
          base::RetainedRef(base::ThreadTaskRunnerHandle::Get()),
          base::Bind(&PrefetchImporterImpl::OnMoveFileDone,
                     weak_ptr_factory_.GetWeakPtr(), offline_page, callback)));
}

void PrefetchImporterImpl::OnMoveFileDone(const OfflinePageItem& offline_page,
                                          const CompletedCallback& callback,
                                          bool success) {
  if (!success) {
    callback.Run(false, OfflinePageModel::kInvalidOfflineId);
    return;
  }

  OfflinePageModel* offline_page_model =
      OfflinePageModelFactory::GetForBrowserContext(context_);
  DCHECK(offline_page_model);

  offline_page_model->AddPage(
      offline_page, base::Bind(&PrefetchImporterImpl::OnPageAdded,
                               weak_ptr_factory_.GetWeakPtr(), callback));
}

void PrefetchImporterImpl::OnPageAdded(const CompletedCallback& callback,
                                       AddPageResult result,
                                       int64_t offline_id) {
  callback.Run(result == AddPageResult::SUCCESS, offline_id);
}

}  // namespace offline_pages
