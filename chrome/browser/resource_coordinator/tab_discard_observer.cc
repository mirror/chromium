// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_discard_observer.h"

namespace resource_coordinator {

void TabDiscardObserver::OnDiscardedStateChange(content::WebContents* contents,
                                                bool is_discarded) {}

void TabDiscardObserver::OnAutoDiscardableStateChange(
    content::WebContents* contents,
    bool is_auto_discardable) {}

TabDiscardObserver::~TabDiscardObserver() = default;

}  // namespace resource_coordinator
