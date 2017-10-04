// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/background_fetch/background_fetch_registration_notifier.h"

#include "base/bind.h"

namespace content {

BackgroundFetchRegistrationNotifier::BackgroundFetchRegistrationNotifier() =
    default;

BackgroundFetchRegistrationNotifier::~BackgroundFetchRegistrationNotifier() =
    default;

void BackgroundFetchRegistrationNotifier::AddObserver(
    int64_t service_worker_registration_id,
    const url::Origin& origin,
    const std::string& id,
    blink::mojom::BackgroundFetchRegistrationObserverPtr observer) {
  int64_t observer_id = next_observer_id_++;

  // Observe connection errors, which occur when the JavaScript object or the
  // renderer hosting them goes away. (For example through navigation.)
  observer.set_connection_error_handler(
      base::BindOnce(&BackgroundFetchRegistrationNotifier::OnConnectionError,
                     base::Unretained(this), observer_id));

  observers_[observer_id] = std::move(observer);
}

void BackgroundFetchRegistrationNotifier::Notify(
    int64_t service_worker_registration_id,
    const url::Origin& origin,
    const std::string& id,
    uint64_t upload_total,
    uint64_t uploaded,
    uint64_t download_total,
    uint64_t downloaded) {
  // TODO(peter): Actually fire observers for the given registration.
}

void BackgroundFetchRegistrationNotifier::OnConnectionError(
    int64_t observer_id) {
  observers_.erase(observer_id);
}

}  // namespace content
