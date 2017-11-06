// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_APP_LIST_ASH_APP_LIST_CONTROLLER_H_
#define ASH_APP_LIST_ASH_APP_LIST_CONTROLLER_H_

#include "ash/public/interfaces/app_list.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "ui/app_list/app_list_model.h"
#include "ui/app_list/app_list_model_observer.h"

namespace app_list {
class AppListModelObserver;
}

namespace ash {

class AshAppListController : public app_list::AppListModelObserver,
                             public mojom::AppListController {
 public:
  AshAppListController();
  ~AshAppListController() override;

  // Binds the app_list::mojom::AppListController interface request to this
  // object.
  void BindRequest(mojom::AppListControllerRequest request);

  // AppListModelObserver:
  void StatusSet(app_list::AppListModel::Status status) override;
  void StateSet(app_list::AppListModel::State state) override;

  // app_list::mojom::AppListModelController:
  void AddObserver(mojom::AppListObserverAssociatedPtrInfo observer) override;
  void SetStatus(app_list::AppListModel::Status status) override;
  void SetState(app_list::AppListModel::State state) override;

 private:
  app_list::AppListModel model_;

  // Bindings for the AppListController interface.
  mojo::BindingSet<mojom::AppListController> bindings_;

  // True when applying changes from the remote AppListModel owned by Chrome.
  // Changes to the local AppListModel should not be reported during this time.
  bool applying_remote_app_list_model_changes_ = false;

  // True if Ash and Chrome should synchronize separate AppListModel instances.
  bool should_synchronize_app_list_models_ = false;

  // The set of app list model observers notified about state and model changes.
  mojo::AssociatedInterfacePtrSet<mojom::AppListObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(AshAppListController);
};

}  // namespace ash

#endif  // ASH_APP_LIST_ASH_APP_LIST_CONTROLLER_H_
