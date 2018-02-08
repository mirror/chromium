// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_PENDING_STATE_UPDATER_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_PENDING_STATE_UPDATER_H_

#include <vector>

#include "components/offline_pages/core/background/request_coordinator.h"
#include "components/offline_pages/core/background/save_page_request.h"

namespace offline_pages {

class PendingStateUpdater {
 public:
  PendingStateUpdater(RequestCoordinator* request_coordinator);
  ~PendingStateUpdater();

  // Set PendingState for available requests when network is loss.
  void UpdateRequestsLossOfNetwork();

  // Set PendingState for the available requests not picked for offlining and
  // notify RequestCoordinator::Observers.
  void UpdateNonPickedRequests(
      const SavePageRequest& picked_request,
      std::vector<std::unique_ptr<SavePageRequest>> available_requests);

  // Set PendingState for multiple requests.
  void SetPendingStateCallback(
      std::vector<std::unique_ptr<SavePageRequest>> requests);

  // Set PendingState for a request.
  void SetPendingState(SavePageRequest& request);

 private:
  RequestCoordinator* request_coordinator_;

  bool requests_pending_another_download_;

  base::WeakPtrFactory<PendingStateUpdater> weak_ptr_factory_;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_PENDING_STATE_UPDATER_H_
