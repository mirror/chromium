// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/browser_dialogs.h"

#include "base/feature_list.h"
#include "chrome/browser/ui/cocoa/task_manager_mac.h"
#include "chrome/browser/ui/views/task_manager_view.h"
#include "chrome/common/chrome_features.h"

namespace {

bool ShouldUseViewsTaskManager() {
  return base::FeatureList::IsEnabled(features::kViewsTaskManager);
}

}  // namespace

namespace chrome {

task_manager::TaskManagerTableModel* ShowTaskManager(Browser* browser) {
  if (ShouldUseViewsTaskManager())
    return task_manager::TaskManagerView::Show(browser);
  return task_manager::TaskManagerMac::Show();
}

void HideTaskManager() {
  if (ShouldUseViewsTaskManager())
    return task_manager::TaskManagerView::Hide();
  task_manager::TaskManagerMac::Hide();
}

}  // namespace chrome
