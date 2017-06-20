// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_REMOTE_OFFLINE_PAGES_TRACKER_H_
#define COMPONENTS_NTP_SNIPPETS_REMOTE_OFFLINE_PAGES_TRACKER_H_

#include <string>

#include "url/gurl.h"

namespace ntp_snippets {

// Synchronously answers whether there is an offline page for a given URL.
class OfflinePagesTracker {
 public:
  virtual ~OfflinePagesTracker() = default;

  virtual bool OfflinePageExists(const GURL url) const = 0;
};

}  // namespace ntp_snippets

#endif  // COMPONENTS_NTP_SNIPPETS_REMOTE_OFFLINE_PAGES_TRACKER_H_
