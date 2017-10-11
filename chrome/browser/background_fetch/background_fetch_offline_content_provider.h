// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_OFFLINE_CONTENT_PROVIDER_H_
#define CHROME_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_OFFLINE_CONTENT_PROVIDER_H_

#include <set>
#include <string>

#include "components/offline_items_collection/core/offline_content_provider.h"

class Profile;

namespace offline_items_collection {
class OfflineContentAggregator;
}

class BackgroundFetchOfflineContentProvider
    : public offline_items_collection::OfflineContentProvider {
 public:
  explicit BackgroundFetchOfflineContentProvider(Profile* profile);
  ~BackgroundFetchOfflineContentProvider() override;

  void StartDownload(const std::string& name);

  // OfflineContentProvider implementation:
  bool AreItemsAvailable() override;
  void OpenItem(const offline_items_collection::ContentId& id) override;
  void RemoveItem(const offline_items_collection::ContentId& id) override;
  void CancelDownload(const offline_items_collection::ContentId& id) override;
  void PauseDownload(const offline_items_collection::ContentId& id) override;
  void ResumeDownload(const offline_items_collection::ContentId& id,
                      bool has_user_gesture) override;
  const offline_items_collection::OfflineItem* GetItemById(
      const offline_items_collection::ContentId& id) override;
  OfflineItemList GetAllItems() override;
  void GetVisualsForItem(const offline_items_collection::ContentId& id,
                         const VisualsCallback& callback) override;
  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;

 private:
  offline_items_collection::OfflineContentAggregator*
      offline_content_aggregator_;

  std::set<Observer*> observers_;

  OfflineItemList items_;
};

#endif  // CHROME_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_OFFLINE_CONTENT_PROVIDER_H_
