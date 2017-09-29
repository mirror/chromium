// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_REGISTRATION_ID_H_
#define CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_REGISTRATION_ID_H_

#include <stdint.h>
#include <string>

#include "content/common/content_export.h"
#include "url/origin.h"

namespace content {

// The Background Fetch registration id corresponds to the information required
// to uniquely identify a Background Fetch registration in scope of a profile.
class CONTENT_EXPORT BackgroundFetchRegistrationId {
 public:
  // Constructs a null ID.
  BackgroundFetchRegistrationId();

  // See corresponding getters for descriptions of |unsafe_id| and |job_guid|.
  BackgroundFetchRegistrationId(int64_t service_worker_registration_id,
                                const url::Origin& origin,
                                const std::string& unsafe_id,
                                const std::string& job_guid);

  // Copyable and movable.
  BackgroundFetchRegistrationId(const BackgroundFetchRegistrationId& other);
  BackgroundFetchRegistrationId(BackgroundFetchRegistrationId&& other);
  BackgroundFetchRegistrationId& operator=(
      const BackgroundFetchRegistrationId& other);
  BackgroundFetchRegistrationId& operator=(
      BackgroundFetchRegistrationId&& other);

  ~BackgroundFetchRegistrationId();

  // Returns whether the |other| registration id are identical or different.
  bool operator==(const BackgroundFetchRegistrationId& other) const;
  bool operator!=(const BackgroundFetchRegistrationId& other) const;

  // Enables this type to be used in an std::map and std::set.
  // TODO(peter): Delete this when we switch away from using maps.
  bool operator<(const BackgroundFetchRegistrationId& other) const;

  // Returns whether this registration id refers to valid data.
  bool is_null() const;

  int64_t service_worker_registration_id() const {
    return service_worker_registration_id_;
  }
  const url::Origin& origin() const { return origin_; }

  // The ID provided by the website. Should not be used as a primary key in any
  // data structures, since not only are they per-ServiceWorkerRegistration, but
  // there can exist two or more different Background Fetch registrations at
  // once with the same |unsafe_id|. For example, if JavaScript holds a
  // reference to a BackgroundFetchRegistration object after that registration
  // is aborted/completed and then the website creates a new registration with
  // the same ID, the original BackgroundFetchRegistration object is still valid
  // and must continue to refer to the old data.
  const std::string& unsafe_id() const { return unsafe_id_; }

  // The globally unique ID for the Background Fetch registration. Values are
  // never re-used. Always use this instead of |unsafe_id| (APIs that receive an
  // |unsafe_id| from the website must convert it to a |job_guid| as soon as
  // possible).
  const std::string& job_guid() const { return job_guid_; }

 private:
  int64_t service_worker_registration_id_;
  url::Origin origin_;
  std::string unsafe_id_;
  std::string job_guid_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_REGISTRATION_ID_H_
