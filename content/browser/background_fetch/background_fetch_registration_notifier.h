// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_REGISTRATION_NOTIFIER_H_
#define CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_REGISTRATION_NOTIFIER_H_

#include <stdint.h>
#include <string>

#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "third_party/WebKit/public/platform/modules/background_fetch/background_fetch.mojom.h"

namespace url {
class Origin;
}

namespace content {

// Tracks the live BackgroundFetchRegistration objects across the renderer
// processes and provides the functionality to notify them of progress updates.
class BackgroundFetchRegistrationNotifier {
 public:
  BackgroundFetchRegistrationNotifier();
  ~BackgroundFetchRegistrationNotifier();

  // Registers the |observer| to be notified when fetches for the registration
  // for the indicated properties progress.
  void AddObserver(
      int64_t service_worker_registration_id,
      const url::Origin& origin,
      const std::string& id,
      blink::mojom::BackgroundFetchRegistrationObserverPtr observer);

  // Notifies any registered observer for the given registration details of
  // the made progress. This will cause JavaScript events to fire.
  void Notify(int64_t service_worker_registration_id,
              const url::Origin& origin,
              const std::string& id,
              uint64_t upload_total,
              uint64_t uploaded,
              uint64_t download_total,
              uint64_t downloaded);

  // Removes the observers for the registration with the given details. When the
  // background fetch was successful, the Notify() function should have been
  // called beforehand to inform developers of the final state.
  void RemoveObservers();

 private:
  // Called when the connection associated with the |observer_id| goes away.
  void OnConnectionError(int64_t observer_id);

  // Id to assiciate with the next observer that will be added. It will continue
  // to be incremented as the selection of observers changes.
  int64_t next_observer_id_ = 0;

  // Storage of the registration observers keyed by their Id.
  base::flat_map<int64_t, blink::mojom::BackgroundFetchRegistrationObserverPtr>
      observers_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundFetchRegistrationNotifier);
};

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_REGISTRATION_NOTIFIER_H_
