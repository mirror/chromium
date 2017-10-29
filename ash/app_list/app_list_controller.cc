// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/app_list/app_list_controller.h"

#include <memory>

namespace ash {

AppListController::AppListController() {
  LOG(ERROR) << "***** AppListController::AppListController";
}

AppListController::~AppListController() {
  LOG(ERROR) << "***** AppListController::~AppListController";
}

void AppListController::BindRequest(mojom::AppListControllerRequest request) {
  LOG(ERROR) << "***** AppListController::BindRequest";
  bindings_.AddBinding(this, std::move(request));
}

void AppListController::SetObserver(
    mojom::AppListObserverAssociatedPtrInfo observer) {
  LOG(ERROR) << "***** AppListController::AddObserver";
  observer_.Bind(std::move(observer));
}

void AppListController::DeleteItem(const std::string& id) {
  LOG(ERROR) << "***** AppListController::DeleteItem";
  observer_->OnDeleteItem(id);
}

}  // namespace ash
