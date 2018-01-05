// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/app_list/controller/app_list_controller_impl.h"

#include <string>
#include <utility>

#include "ash/app_list/model/app_list_model.h"
#include "ash/app_list/model/search/search_model.h"

namespace ash {

// static
AppListControllerImpl* AppListControllerImpl::instance_ = nullptr;

AppListControllerImpl::AppListControllerImpl(
    app_list::AppListModel* app_list_model,
    app_list::SearchModel* search_model)
    : app_list_model_(app_list_model), search_model_(search_model) {
  DCHECK(!instance_);
  instance_ = this;
  DCHECK(app_list_model_);
  DCHECK(search_model_);
}

AppListControllerImpl::~AppListControllerImpl() {
  if (instance_ == this)
    instance_ = nullptr;
}

void AppListControllerImpl::SetClient(mojom::AppListControllerClient* client) {
  app_list_client_ = client;
}

void AppListControllerImpl::ActivateItem(const std::string& id,
                                         int event_flags) {
  if (app_list_client_)
    app_list_client_->ActivateItem(id, event_flags);
}

}  // namespace ash
