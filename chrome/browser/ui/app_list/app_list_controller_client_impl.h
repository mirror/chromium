// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_APP_LIST_CONTROLLER_CLIENT_IMPL_H_
#define CHROME_BROWSER_UI_APP_LIST_APP_LIST_CONTROLLER_CLIENT_IMPL_H_

#include <string>

#include "ash/app_list/controller/app_list_controller_impl.h"
#include "ash/public/interfaces/app_list.mojom.h"
#include "base/macros.h"

// This class implements methods called from Ash into Chrome for app list.
class AppListControllerClientImpl : public ash::mojom::AppListControllerClient {
 public:
  explicit AppListControllerClientImpl(
      ash::AppListControllerImpl* app_list_controller);
  ~AppListControllerClientImpl() override;

  // Returns the single AppListControllerClientImpl instance.
  static AppListControllerClientImpl* Get() { return instance_; }

  // ash::mojom::AppListControllerClient:
  void ActivateItem(const std::string& id, int event_flags) override;

 private:
  static AppListControllerClientImpl* instance_;

  // App list controller mojo service in ash.
  ash::AppListControllerImpl* app_list_controller_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(AppListControllerClientImpl);
};

#endif  // CHROME_BROWSER_UI_APP_LIST_APP_LIST_CONTROLLER_CLIENT_IMPL_H_
