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
    const std::string& unique_id,
    blink::mojom::BackgroundFetchRegistrationObserverPtr observer) {
  // Observe connection errors, which occur when the JavaScript object or the
  // renderer hosting them goes away. (For example through navigation.)
  observer.set_connection_error_handler(
      base::BindOnce(&BackgroundFetchRegistrationNotifier::OnConnectionError,
                     base::Unretained(this), unique_id, observer.get()));

  observers_[unique_id].push_back(std::move(observer));
}

void BackgroundFetchRegistrationNotifier::Notify(const std::string& unique_id,
                                                 uint64_t upload_total,
                                                 uint64_t uploaded,
                                                 uint64_t download_total,
                                                 uint64_t downloaded) {
  auto registration_observers_iter = observers_.find(unique_id);
  if (registration_observers_iter == observers_.end())
    return;

  for (const auto& observer : registration_observers_iter->second)
    observer->OnProgress(upload_total, uploaded, download_total, downloaded);
}

void BackgroundFetchRegistrationNotifier::RemoveObservers(
    const std::string& unique_id) {
  observers_.erase(unique_id);
}

void BackgroundFetchRegistrationNotifier::OnConnectionError(
    const std::string& unique_id,
    blink::mojom::BackgroundFetchRegistrationObserver* observer) {
  DCHECK_EQ(observers_.count(unique_id), 1u);
  auto& registration_observers = observers_[unique_id];

  registration_observers.erase(std::remove_if(
      registration_observers.begin(), registration_observers.end(),
      [observer](
          const blink::mojom::BackgroundFetchRegistrationObserverPtr& ptr) {
        return ptr.get() == observer;
      }));

  if (!registration_observers.size())
    observers_.erase(unique_id);
}

}  // namespace content
