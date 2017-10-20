// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_MODEL_GET_PAGES_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_MODEL_GET_PAGES_TASK_H_

#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/client_id.h"
#include "components/offline_pages/core/offline_page_item.h"
#include "components/offline_pages/core/offline_page_metadata_store_sql.h"
#include "components/offline_pages/core/offline_page_types.h"
#include "components/offline_pages/core/task.h"

class GURL;

namespace offline_pages {
class ClientPolicyController;

// Gets offline pages that match the list of client IDs.
class GetPagesTask : public Task {
 public:
  // Structure defining and intermediate read result.
  struct ReadResult {
    ReadResult();
    ReadResult(const ReadResult& other);
    ~ReadResult();

    bool success;
    std::vector<OfflinePageItem> pages;
  };

  using DbWorkCallback = OfflinePageMetadataStoreSQL::RunCallback<ReadResult>;

  // Creates |GetPagesTask| reading all pages from DB.
  static std::unique_ptr<GetPagesTask> CreateTaskMatchingAllPages(
      OfflinePageMetadataStoreSQL* store,
      const MultipleOfflinePageItemCallback& callback);

  // Creates |GetPagesTask| reading pages matching provided |client_ids| from
  // DB.
  static std::unique_ptr<GetPagesTask> CreateTaskMatchingClientIds(
      OfflinePageMetadataStoreSQL* store,
      const std::vector<ClientId>& client_ids,
      const MultipleOfflinePageItemCallback& callback);

  // Creates |GetPagesTask| reading pages belonging to provided |name_space|
  // from DB.
  static std::unique_ptr<GetPagesTask> CreateTaskMatchingNamespace(
      OfflinePageMetadataStoreSQL* store,
      const std::string& name_space,
      const MultipleOfflinePageItemCallback& callback);

  // Creates |GetPagesTask| reading pages removed on cache reset from DB.
  static std::unique_ptr<GetPagesTask>
  CreateTaskMatchingPagesRemovedOnCacheReset(
      OfflinePageMetadataStoreSQL* store,
      ClientPolicyController* policy_controller,
      const MultipleOfflinePageItemCallback& callback);

  // Creates |GetPagesTask| reading pages in namespaces supported by downloads
  // from DB.
  static std::unique_ptr<GetPagesTask>
  CreateTaskMatchingPagesSupportedByDownloads(
      OfflinePageMetadataStoreSQL* store,
      ClientPolicyController* policy_controller,
      const MultipleOfflinePageItemCallback& callback);

  // Creates |GetPagesTask| reading pages matching provided |request_origin|
  // from DB.
  static std::unique_ptr<GetPagesTask> CreateTaskMatchingRequestOrigin(
      OfflinePageMetadataStoreSQL* store,
      const std::string& request_origin,
      const MultipleOfflinePageItemCallback& callback);

  // Creates |GetPagesTask| reading pages matching provided |url| from DB.
  // The url will be matched against original URL and final URL. Fragments will
  // be removed from all URLs prior to matching. Only a match on a single field
  // is necessary.
  static std::unique_ptr<GetPagesTask> CreateTaskMatchingUrl(
      OfflinePageMetadataStoreSQL* store,
      const GURL& url,
      const MultipleOfflinePageItemCallback& callback);

  // Creates |GetPagesTask| reading a single page matching provided |offline_id|
  // from DB.
  static std::unique_ptr<GetPagesTask> CreateTaskMatchingOfflineId(
      OfflinePageMetadataStoreSQL* store,
      int64_t offline_id,
      const SingleOfflinePageItemCallback& callback);

  ~GetPagesTask() override;

  // Task implementation:
  void Run() override;

 private:
  GetPagesTask(OfflinePageMetadataStoreSQL* store,
               DbWorkCallback db_work_callback,
               const MultipleOfflinePageItemCallback& callback);

  void ReadRequests();
  void CompleteWithResult(ReadResult result);

  OfflinePageMetadataStoreSQL* store_;
  DbWorkCallback db_work_callback_;
  MultipleOfflinePageItemCallback callback_;

  base::WeakPtrFactory<GetPagesTask> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(GetPagesTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_MODEL_GET_PAGES_TASK_H_
