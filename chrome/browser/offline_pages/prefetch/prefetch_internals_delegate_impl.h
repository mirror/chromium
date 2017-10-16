// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_PREFETCH_INTERNALS_DELEGATE_IMPL_H_
#define CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_PREFETCH_INTERNALS_DELEGATE_IMPL_H_

#include <memory>

#include "base/macros.h"

namespace offline_pages {

// The implementation of PrefetchInternalsDelegate.
class PrefetchInternalsDelegateImpl : public PrefetchInternalsDelegate {
 public:
  PrefetchInternalsDelegateImpl();
  ~PrefetchInternalsDelegateImpl() override;

  // PrefetchInternalsDelegate implementation.
  void ShowNotificationForDebugging() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PrefetchInternalsDelegateImpl);
};

}  // namespace offline_pages

#endif  // CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_PREFETCH_INTERNALS_DELEGATE_IMPL_H_
