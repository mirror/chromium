// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/background_fetch/background_fetch_offline_content_provider.h"

#include "chrome/browser/offline_items_collection/offline_content_aggregator_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/offline_items_collection/core/offline_content_aggregator.h"
#include "components/offline_items_collection/core/offline_item.h"
#include "content/public/browser/browser_thread.h"

BackgroundFetchOfflineContentProvider::BackgroundFetchOfflineContentProvider(
    Profile* profile)
    : offline_content_aggregator_(
          offline_items_collection::OfflineContentAggregatorFactory::
              GetForBrowserContext(profile)) {
  offline_content_aggregator_->RegisterProvider("background_fetch", this);
}

BackgroundFetchOfflineContentProvider::
    ~BackgroundFetchOfflineContentProvider() {}

bool BackgroundFetchOfflineContentProvider::AreItemsAvailable() {
  return true;
}

void BackgroundFetchOfflineContentProvider::OpenItem(
    const offline_items_collection::ContentId& id) {}

void BackgroundFetchOfflineContentProvider::RemoveItem(
    const offline_items_collection::ContentId& id) {
  for (auto it = items_.begin(); it != items_.end(); ++it) {
    if (it->id == id) {
      items_.erase(it);
      break;
    }
  }
}

void BackgroundFetchOfflineContentProvider::CancelDownload(
    const offline_items_collection::ContentId& id) {}

void BackgroundFetchOfflineContentProvider::PauseDownload(
    const offline_items_collection::ContentId& id) {}

void BackgroundFetchOfflineContentProvider::ResumeDownload(
    const offline_items_collection::ContentId& id,
    bool has_user_gesture) {}

const offline_items_collection::OfflineItem*
BackgroundFetchOfflineContentProvider::GetItemById(
    const offline_items_collection::ContentId& id) {
  for (auto& item : items_) {
    if (item.id == id) {
      return &item;
    }
  }
  return nullptr;
}

BackgroundFetchOfflineContentProvider::OfflineItemList
BackgroundFetchOfflineContentProvider::GetAllItems() {
  return items_;
}

void BackgroundFetchOfflineContentProvider::GetVisualsForItem(
    const offline_items_collection::ContentId& id,
    const VisualsCallback& callback) {
  content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
                                   base::BindOnce(callback, id, nullptr));
}

void BackgroundFetchOfflineContentProvider::AddObserver(Observer* observer) {
  observers_.insert(observer);
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          [](Observer* observer,
             BackgroundFetchOfflineContentProvider* provider) {
            observer->OnItemsAvailable(provider);
          },
          observer, this));
}

void BackgroundFetchOfflineContentProvider::RemoveObserver(Observer* observer) {
  observers_.erase(observer);
}

void BackgroundFetchOfflineContentProvider::StartDownload(
    const std::string& name) {
  offline_items_collection::OfflineItem item(
      offline_items_collection::ContentId("background_fetch", name));
  item.title = name;
  item.description = "More detailed item: " + name;
  item.is_transient = false;
  item.state = offline_items_collection::OfflineItemState::IN_PROGRESS;
  items_.push_back(item);

  for (auto* observer : observers_) {
    observer->OnItemsAdded({item});
  }
}
