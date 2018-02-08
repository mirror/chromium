// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/mock_download_item.h"

namespace content {

MockDownloadItem::MockDownloadItem() {}

MockDownloadItem::~MockDownloadItem() {
  for (auto& observer : observers_)
    observer.OnDownloadDestroyed(this);
}

void MockDownloadItem::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void MockDownloadItem::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void MockDownloadItem::SetDelegate(
    std::unique_ptr<download::DownloadItemDelegate> delegate) {
  delegate_ = std::move(delegate);
}

download::DownloadItemDelegate* MockDownloadItem::GetDelegate() const {
  return delegate_.get();
}

void MockDownloadItem::NotifyObserversDownloadOpened() {
  for (auto& observer : observers_)
    observer.OnDownloadOpened(this);
}

void MockDownloadItem::NotifyObserversDownloadRemoved() {
  for (auto& observer : observers_)
    observer.OnDownloadRemoved(this);
}

void MockDownloadItem::NotifyObserversDownloadUpdated() {
  for (auto& observer : observers_)
    observer.OnDownloadUpdated(this);
}

}
