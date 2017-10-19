// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_pages/prefetch/prefetch_internals_delegate_impl.h"

#include "chrome/browser/offline_pages/prefetch/prefetched_pages_notifier.h"
#include "url/gurl.h"

namespace offline_pages {

PrefetchInternalsDelegateImpl::PrefetchInternalsDelegateImpl() = default;
PrefetchInternalsDelegateImpl::~PrefetchInternalsDelegateImpl() = default;

void PrefetchInternalsDelegateImpl::ShowNotificationForDebugging() {
  offline_pages::ShowPrefetchedContentNotification(
      GURL("https://www.example.com"));
}

}  // namespace offline_pages
