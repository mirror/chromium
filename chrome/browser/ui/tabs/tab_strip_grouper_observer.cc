// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/tabs/tab_strip_grouper_observer.h"

TabStripGrouperObserver::TabStripGrouperObserver() = default;
TabStripGrouperObserver::~TabStripGrouperObserver() = default;

void TabStripGrouperObserver::ItemsChanged(const std::vector<int>& removed,
                                           const std::vector<int>& added) {}

void TabStripGrouperObserver::ItemUpdatedAt(int index,
                                            TabChangeType change_type) {}

void TabStripGrouperObserver::ItemSelectionChanged(
    const ui::ListSelectionModel& old_model) {}

void TabStripGrouperObserver::ItemNeedsAttentionAt(int index) {}
