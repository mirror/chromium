// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_CHROME_APP_LIST_MODEL_OBSERVER_H_
#define CHROME_BROWSER_UI_APP_LIST_CHROME_APP_LIST_MODEL_OBSERVER_H_

#include <string>

#include "ash/app_list/model/ash_app_list_model_observer.h"
#include "base/macros.h"

class ChromeAppListModelUpdater;

class ChromeAppListModelObserver : public AshAppListModelObserver {
 public:
  explicit ChromeAppListModelObserver(ChromeAppListModelUpdater* model_updater);
  ~ChromeAppListModelObserver() override;

  void ActivateItem(const std::string& id, int event_flags) override;
  ui::MenuModel* GetContextMenuModel(const std::string& id) override;

 private:
  ChromeAppListModelUpdater* model_updater_;  // Unowned

  DISALLOW_COPY_AND_ASSIGN(ChromeAppListModelObserver);
};

#endif  // CHROME_BROWSER_UI_APP_LIST_CHROME_APP_LIST_MODEL_OBSERVER_H_
