// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/tabs/tab_strip_grouper_observer.h"

TabStripGrouperObserver::TabStripGrouperObserver() = default;
TabStripGrouperObserver::~TabStripGrouperObserver() = default;

void TabStripGrouperObserver::ItemsInsertedAt(TabStripGrouper* grouper,
                                              int begin_index,
                                              int count) {}

void TabStripGrouperObserver::ItemsClosingAt(TabStripGrouper* grouper,
                                             int begin_index,
                                             int count) {}

void TabStripGrouperObserver::ItemNeedsAttentionAt(int index) {}
