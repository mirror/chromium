// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/offline_page_archiver.h"

#include <errno.h>

#include "base/strings/utf_string_conversions.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "components/offline_pages/core/model/offline_page_model_taskified.h"
#include "components/offline_pages/core/offline_store_utils.h"
#include "components/offline_pages/core/system_download_manager.h"

namespace offline_pages {

void OfflinePageArchiver::PublishPage(
    OfflinePageItem offline_page,
    const base::FilePath public_directory,
    SystemDownloadManager* download_manager,
    scoped_refptr<MoveAndAddResults> move_results) {
  // Calcualte the new file name.
  // TODO(petewil) - We will want to add code to make sure the filename is
  // unique.
  base::FilePath new_file_path =
      public_directory.Append(offline_page.file_path.BaseName());

  // Move on the file thread, synchronously.
  bool moved = base::Move(offline_page.file_path, new_file_path);
  if (!moved) {
    move_results->set_move_result(SavePageResult::FILE_MOVE_FAILED);
    return;
  }
  // Update the offline page item with the new path now that the file has been
  // moved.
  offline_page.file_path = new_file_path;

  // Tell the download manager about our file, get back an id.
  if (!download_manager->IsDownloadManagerInstalled()) {
    move_results->set_move_result(
        SavePageResult::ADD_TO_DOWNLOAD_MANAGER_FAILED);
    return;
  }
  // We use the title for a description, since the add to the download manager
  // fails without a description, and we don't have anything better to use.
  int64_t download_id = download_manager->AddCompletedDownload(
      base::UTF16ToUTF8(offline_page.title),
      base::UTF16ToUTF8(offline_page.title),
      store_utils::ToDatabaseFilePath(offline_page.file_path),
      offline_page.file_size, offline_page.url.spec(), std::string());
  if (download_id == 0LL) {
    move_results->set_move_result(
        SavePageResult::ADD_TO_DOWNLOAD_MANAGER_FAILED);
    return;
  }

  // Put results into the result object.
  move_results->set_move_result(SavePageResult::SUCCESS);
  move_results->set_new_file_path(new_file_path);
  move_results->set_download_id(download_id);

  return;
}

void OfflinePageArchiver::SavePageToDownloads(
    OfflinePageItem offline_page,
    const SavePageCallback& save_page_callback,
    const scoped_refptr<base::SequencedTaskRunner>& background_task_runner,
    ArchiveManager* archive_manager,
    SystemDownloadManager* download_manager,
    base::WeakPtr<OfflinePageModelTaskified> model_weak_ptr) {
  scoped_refptr<MoveAndAddResults> move_results = new MoveAndAddResults();
  background_task_runner->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(&OfflinePageArchiver::PublishPage, offline_page,
                     archive_manager->GetPublicArchivesDir(), download_manager,
                     move_results),
      base::BindOnce(&OfflinePageModelTaskified::MoveAndAddDone, model_weak_ptr,
                     save_page_callback, offline_page, move_results));
}

}  // namespace offline_pages
