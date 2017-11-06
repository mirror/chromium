// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/chrome_app_list_controller.h"

#include <utility>

// static
ChromeAppListController* ChromeAppListController::instance_ = nullptr;

ChromeAppListController::ChromeAppListController(ash::AppListModel* model)
    : model_(model), observer_binding_(this), weak_ptr_factory_(this) {
  DCHECK(!instance_);
  instance_ = this;

  // AppListModel initializes the app list item.
  DCHECK(model_);

  // Start observing the app list controller.
  if (ConnectToAppListController()) {
    ash::mojom::AppListObserverAssociatedPtrInfo ptr_info;
    observer_binding_.Bind(mojo::MakeRequest(&ptr_info));
    app_list_controller_->AddObserver(std::move(ptr_info));
  }

  model_->AddObserver(this);
}

ChromeAppListController::~ChromeAppListController() {
  model_->RemoveObserver(this);

  if (instance_ == this)
    instance_ = nullptr;
}

bool ChromeAppListController::ConnectToAppListController() {
  // Synchronization is required in the Mash config, since Chrome and Ash run in
  // separate processes; it's optional via kAshDisableShelfModelSynchronization
  // in the Classic Ash config, where Chrome can uses Ash's AppListModel
  // directly.
  if (!ash_util::IsRunningInMash() &&
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          ash::switches::kAshDisableShelfModelSynchronization)) {
    return false;
  }

  if (app_list_controller_.is_bound())
    return true;

  auto* connection = content::ServiceManagerConnection::GetForProcess();
  auto* connector = connection ? connection->GetConnector() : nullptr;
  // Unit tests may not have a connector.
  if (!connector)
    return false;

  connector->BindInterface(ash::mojom::kServiceName, &app_list_controller_);
  return true;
}

///////////////////////////////////////////////////////////////////////////////
// ash::mojom::AppListObserver:

void ChromeAppListController::OnStatusSet(AppListModelStatus status) {
  DCHECK(app_list_controller_) << " Unexpected model sync";
  DCHECK(!applying_remote_app_list_model_changes_)
      << " Unexpected model change";

  base::AutoReset<bool> reset(&applying_remote_app_list_model_changes_, true);
  model_->SetStatus(status);
}

void ChromeAppListController::OnStateSet(AppListModelState state) {
  DCHECK(app_list_controller_) << " Unexpected model sync";
  DCHECK(!applying_remote_app_list_model_changes_)
      << " Unexpected model change";

  base::AutoReset<bool> reset(&applying_remote_app_list_model_changes_, true);
  model_->SetState(state);
}

///////////////////////////////////////////////////////////////////////////////
// ash::AppListModelObserver:

void ChromeAppListController::StatusSet(AppListModelStatus status) {
  if (app_list_controller_ && !applying_remote_app_list_model_changes_)
    app_list_controller_->SetStatus(status);
}

void ChromeAppListController::StatusSet(AppListModelState state) {
  if (app_list_controller_ && !applying_remote_app_list_model_changes_)
    app_list_controller_->SetState(state);
}
