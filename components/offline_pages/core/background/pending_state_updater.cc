// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/background/pending_state_updater.h"

#include "components/offline_items_collection/core/pending_state.h"

namespace offline_pages {

PendingStateUpdater::PendingStateUpdater(
    RequestCoordinator* request_coordinator)
    : request_coordinator_(request_coordinator),
      requests_pending_another_download_(false),
      weak_ptr_factory_(this) {}

PendingStateUpdater::~PendingStateUpdater() {}

void PendingStateUpdater::UpdateRequestsLossOfNetwork() {
  requests_pending_another_download_ = false;
  request_coordinator_->GetAllRequests(
      base::Bind(&PendingStateUpdater::SetPendingStateCallback,
                 weak_ptr_factory_.GetWeakPtr()));
}

void PendingStateUpdater::UpdateNonPickedRequests(
    const SavePageRequest& picked_request,
    std::vector<std::unique_ptr<SavePageRequest>> available_requests) {
  // Requests do not need to be updated.
  if (requests_pending_another_download_)
    return;

  // All available requests expect for the picked request changed to waiting
  // for another download to complete.
  requests_pending_another_download_ = true;
  for (auto& request : available_requests) {
    if (*request == picked_request)
      continue;
    request->set_pending_state(PendingState::PENDING_ANOTHER_DOWNLOAD);
    request_coordinator_->NotifyChanged(*request);
  }
}

void PendingStateUpdater::SetPendingStateCallback(
    std::vector<std::unique_ptr<SavePageRequest>> requests) {
  for (auto& request : requests) {
    SetPendingState(*request.get());
    request_coordinator_->NotifyChanged(*request);
  }
}

void PendingStateUpdater::SetPendingState(SavePageRequest& request) {
  if (request.request_state() == SavePageRequest::RequestState::AVAILABLE) {
    if (request_coordinator_->state() ==
        RequestCoordinator::RequestCoordinatorState::OFFLINING) {
      request.set_pending_state(PendingState::PENDING_ANOTHER_DOWNLOAD);
    } else {
      request.set_pending_state(PendingState::PENDING_NETWORK);
    }
  }
}

}  // namespace offline_pages
