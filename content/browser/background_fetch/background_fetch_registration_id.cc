// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/background_fetch/background_fetch_registration_id.h"

#include <tuple>

#include "content/common/service_worker/service_worker_types.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/service_worker_registration.mojom.h"

namespace content {

BackgroundFetchRegistrationId::BackgroundFetchRegistrationId()
    : service_worker_registration_id_(
          blink::mojom::kInvalidServiceWorkerRegistrationId) {}

BackgroundFetchRegistrationId::BackgroundFetchRegistrationId(
    int64_t service_worker_registration_id,
    const url::Origin& origin,
    const std::string& unsafe_id,
    const std::string& job_guid)
    : service_worker_registration_id_(service_worker_registration_id),
      origin_(origin),
      unsafe_id_(unsafe_id),
      job_guid_(job_guid) {
  DCHECK_NE(blink::mojom::kInvalidServiceWorkerRegistrationId,
            service_worker_registration_id);
  DCHECK(!job_guid_.empty());
}

BackgroundFetchRegistrationId::BackgroundFetchRegistrationId(
    const BackgroundFetchRegistrationId& other) = default;

BackgroundFetchRegistrationId::BackgroundFetchRegistrationId(
    BackgroundFetchRegistrationId&& other) = default;

BackgroundFetchRegistrationId& BackgroundFetchRegistrationId::operator=(
    const BackgroundFetchRegistrationId& other) = default;

BackgroundFetchRegistrationId& BackgroundFetchRegistrationId::operator=(
    BackgroundFetchRegistrationId&& other) = default;

BackgroundFetchRegistrationId::~BackgroundFetchRegistrationId() = default;

bool BackgroundFetchRegistrationId::operator==(
    const BackgroundFetchRegistrationId& other) const {
  return job_guid_ == other.job_guid_;
}

bool BackgroundFetchRegistrationId::operator!=(
    const BackgroundFetchRegistrationId& other) const {
  return job_guid_ != other.job_guid_;
}

bool BackgroundFetchRegistrationId::operator<(
    const BackgroundFetchRegistrationId& other) const {
  return job_guid_ < other.job_guid_;
}

bool BackgroundFetchRegistrationId::is_null() const {
  return job_guid_.empty();
}

}  // namespace content
