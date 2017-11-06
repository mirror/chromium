// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_CHROME_APP_LIST_CONTROLLER_H_
#define CHROME_BROWSER_UI_APP_LIST_CHROME_APP_LIST_CONTROLLER_H_

#include <memory>
#include <string>
#include <vector>

#include "ash/public/cpp/app_list_model_observer.h"
#include "ash/public/interfaces/app_list.mojom.h"
#include "base/auto_reset.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"

namespace ash {
class AppListModel;
}  // namespace ash

// ChromeAppListController helps manage Ash's app list for Chrome prefs and
// apps. It helps synchronize app list state with profile preferences.
class ChromeAppListController : private ash::mojom::AppListObserver,
                                private ash::AppListModelObserver {
 public:
  explicit ChromeAppListController(ash::AppListModel* model);
  ~ChromeAppListController() override;

 protected:
  // Connects or reconnects to the mojom::App_listController interface in ash.
  // Returns true if connected; virtual for unit tests.
  virtual bool ConnectToAppListController();

 private:
  // ash::mojom::AppListObserver:
  void OnStatusSet(AppListModelStatus status) override;
  void OnStateSet(AppListModelState state) override;

  // ash::AppListModelObserver:
  void StatusSet(AppListModelStatus status) override;
  void StateSet(AppListModelState state) override;

  static ChromeAppListController* instance_;

  // In classic Ash, this the AppListModel owned by ash::Shell's
  // AshAppListController. In the mash config, this is a separate AppListModel
  // instance, owned by ChromeBrowserMainExtraPartsAsh, and synchronized with
  // Ash's AppListModel.
  ash::AppListModel* model_;

  // Ash's mojom::AppListControllerPtr used to change app list state.
  ash::mojom::AppListControllerPtr app_list_controller_;

  // The binding this instance uses to implment mojom::AppListObserver
  mojo::AssociatedBinding<ash::mojom::AppListObserver> observer_binding_;

  // True when applying changes from the remote AppListModel owned by Ash.
  // Changes to the local AppListModel should not be reported during this time.
  bool applying_remote_app_list_model_changes_ = false;

  base::WeakPtrFactory<ChromeAppListController> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ChromeAppListController);
};

#endif  // CHROME_BROWSER_UI_APP_LIST_CHROME_APP_LIST_CONTROLLER_H_
