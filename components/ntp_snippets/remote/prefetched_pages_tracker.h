// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_REMOTE_PREFETCHED_PAGES_TRACKER_H_
#define COMPONENTS_NTP_SNIPPETS_REMOTE_PREFETCHED_PAGES_TRACKER_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "components/ntp_snippets/remote/offline_pages_tracker.h"
#include "components/offline_pages/core/offline_page_model.h"
#include "url/gurl.h"

namespace offline_pages {
struct OfflinePageItem;
}

namespace ntp_snippets {

// Synchronously answers whether there is a prefetched offline page for a given
// URL.
class PrefetchedPagesTracker
    : public OfflinePagesTracker,
      public offline_pages::OfflinePageModel::Observer {
 public:
  PrefetchedPagesTracker(offline_pages::OfflinePageModel* offline_page_model);
  ~PrefetchedPagesTracker() override;

  // OfflinePagesTracker implementation
  bool OfflinePageExists(const GURL url) const override;

  // OfflinePageModel::Observer implementation.
  void OfflinePageModelLoaded(offline_pages::OfflinePageModel* model) override;
  void OfflinePageAdded(
      offline_pages::OfflinePageModel* model,
      const offline_pages::OfflinePageItem& added_page) override;
  void OfflinePageDeleted(int64_t offline_id,
                          const offline_pages::ClientId& client_id) override;

 private:
  void Initialize(offline_pages::OfflinePageModel* offline_page_model,
                  const std::vector<offline_pages::OfflinePageItem>&
                      all_prefetched_offline_pages);
  void AddOfflinePage(const offline_pages::OfflinePageItem& offline_page_item);

  std::set<GURL> prefetched_urls_;
  std::map<int64_t, GURL> offline_id_to_url_mapping_;

  base::WeakPtrFactory<PrefetchedPagesTracker> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PrefetchedPagesTracker);
};

}  // namespace ntp_snippets

#endif  // COMPONENTS_NTP_SNIPPETS_REMOTE_PREFETCHED_PAGES_TRACKER_H_
