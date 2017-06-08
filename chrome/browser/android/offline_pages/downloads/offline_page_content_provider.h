// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_OFFLINE_PAGES_DOWNLOADS_OFFLINE_PAGE_CONTENT_PROVIDER_H_
#define CHROME_BROWSER_ANDROID_OFFLINE_PAGES_DOWNLOADS_OFFLINE_PAGE_CONTENT_PROVIDER_H_

#include "base/macros.h"
#include "base/supports_user_data.h"
#include "components/offline_items_collection/core/offline_content_provider.h"
#include "components/offline_pages/core/downloads/download_ui_adapter.h"

using ContentId = offline_items_collection::ContentId;
using OfflineItem = offline_items_collection::OfflineItem;
using OfflineContentProvider = offline_items_collection::OfflineContentProvider;

namespace content {
class BrowserContext;
}

namespace offline_pages {
namespace android {

// TODO(shaktisahu): comments.
class OfflinePageContentProvider : public OfflineContentProvider,
                                   public DownloadUIAdapter::Observer,
                                   public base::SupportsUserData::Data {
 public:
  OfflinePageContentProvider(DownloadUIAdapter* download_ui_adapter,
                             content::BrowserContext* browser_context);
  ~OfflinePageContentProvider() override;

  static std::unique_ptr<OfflinePageContentProvider> Create(
      content::BrowserContext* browser_context);

  // OfflineContentProvider implmentation.
  bool AreItemsAvailable() override;
  void OpenItem(const ContentId& id) override;
  void RemoveItem(const ContentId& id) override;
  void CancelDownload(const ContentId& id) override;
  void PauseDownload(const ContentId& id) override;
  void ResumeDownload(const ContentId& id) override;
  const OfflineItem* GetItemById(const ContentId& id) override;
  std::vector<OfflineItem> GetAllItems() override;
  void GetVisualsForItem(const ContentId& id,
                         const VisualsCallback& callback) override{};
  void AddObserver(OfflineContentProvider::Observer* observer) override;
  void RemoveObserver(OfflineContentProvider::Observer* observer) override;

  // DownloadUIAdapter::Observer implementation.
  void ItemsLoaded() override;
  void ItemAdded(const OfflineItem& item) override;
  void ItemUpdated(const OfflineItem& item) override;
  void ItemDeleted(const ContentId& id) override;

 private:
  DownloadUIAdapter* download_ui_adapter_;
  // Not owned.
  content::BrowserContext* browser_context_;
  base::ObserverList<OfflineContentProvider::Observer> observers_;

  bool items_available_ = false;

  DISALLOW_COPY_AND_ASSIGN(OfflinePageContentProvider);
};

}  // namespace android
}  // namespace offline_pages

#endif  // CHROME_BROWSER_ANDROID_OFFLINE_PAGES_DOWNLOADS_OFFLINE_PAGE_CONTENT_PROVIDER_H_
