// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/app_list/ash_app_list_controller.h"

#include <utility>

#include "ash/public/cpp/ash_switches.h"
#include "ash/public/cpp/config.h"
#include "ash/shell.h"
#include "base/command_line.h"

namespace ash {

AshAppListController::AshAppListController() {
  // Synchronization is required in the Mash config, since Chrome and Ash run in
  // separate processes; it's optional via kAshDisableShelfModelSynchronization
  // in the Classic Ash config, where Chrome can uses Ash's AppListModel
  // directly.
  should_synchronize_app_list_models_ =
      Shell::GetAshConfig() == Config::MASH ||
      !base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kAshDisableShelfModelSynchronization);
  model_.AddObserver(this);
}

AshAppListController::~AshAppListController() {
  model_.RemoveObserver(this);
}

// Binds the app_list::mojom::AppListController interface request to this
// object.
void AshAppListController::BindRequest(
    mojom::AppListControllerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

///////////////////////////////////////////////////////////////////////////////
// ash::AppListModelObserver:

void AshAppListController::StatusSet(app_list::AppListModel::Status status) {
  if (applying_remote_app_list_model_changes_ ||
      !should_synchronize_app_list_models_)
    return;

  observers_.ForAllPtrs([status](mojom::AppListObserver* observer) {
    observer->OnStatusSet(status);
  });
}

void AshAppListController::StateSet(app_list::AppListModel::State state) {
  if (applying_remote_app_list_model_changes_ ||
      !should_synchronize_app_list_models_)
    return;

  observers_.ForAllPtrs([state](mojom::AppListObserver* observer) {
    observer->OnStateSet(state);
  });
}

///////////////////////////////////////////////////////////////////////////////
// app_list::mojom::AppListModelController:

void AshAppListController::AddObserver(
    mojom::AppListObserverAssociatedPtrInfo observer) {
  mojom::AppListObserverAssociatedPtr observer_ptr;
  observer_ptr.Bind(std::move(observer));

  if (should_synchronize_app_list_models_) {
    // TODO(hejq):
    // Synchronize two AppListModel instances, one each owned by Ash and Chrome.
    // Notify Chrome of existing AppListModel items and delegates created by
    // Ash.
  }
  observers_.AddPtr(std::move(observer_ptr));
}

void AshAppListController::SetStatus(app_list::AppListModel::Status status) {
  DCHECK(should_synchronize_app_list_models_) << " Unexpected model sync";
  DCHECK(!applying_remote_app_list_model_changes_)
      << " Unexpected model change";
  base::AutoReset<bool> reset(&applying_remote_app_list_model_changes_, true);
  model_.SetStatus(status);
}

void AshAppListController::SetState(app_list::AppListModel::State state) {
  DCHECK(should_synchronize_app_list_models_) << " Unexpected model sync";
  DCHECK(!applying_remote_app_list_model_changes_)
      << " Unexpected model change";
  base::AutoReset<bool> reset(&applying_remote_app_list_model_changes_, true);
  model_.SetState(state);
}

}  // namespace ash
