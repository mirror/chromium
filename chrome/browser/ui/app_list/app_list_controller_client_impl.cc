// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/app_list_controller_client_impl.h"

#include <string>
#include <utility>

// static
AppListControllerClientImpl* AppListControllerClientImpl::instance_ = nullptr;

AppListControllerClientImpl::AppListControllerClientImpl(
    ash::AppListControllerImpl* app_list_controller)
    : app_list_controller_(app_list_controller) {
  DCHECK(!instance_);
  instance_ = this;

  app_list_controller_->SetClient(this);
}

AppListControllerClientImpl::~AppListControllerClientImpl() {
  if (instance_ == this)
    instance_ = nullptr;
}

void AppListControllerClientImpl::ActivateItem(const std::string& id,
                                               int event_flags) {
  // TODO(hejq): Implement this.
}
