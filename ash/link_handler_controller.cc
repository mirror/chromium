// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/link_handler_controller.h"

#include "ash/link_handler_model.h"
#include "ash/link_handler_model_factory.h"
#include "ash/shell.h"

namespace ash {

LinkHandlerController::LinkHandlerController() = default;
LinkHandlerController::~LinkHandlerController() = default;

void LinkHandlerController::BindRequest(
    mojom::LinkHandlerControllerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void LinkHandlerController::ConnectModel(
    mojom::LinkHandlerObserverAssociatedPtrInfo observer,
    const GURL& url) {
  url_ = GURL();
  model_.reset();
  if (Shell::Get()->link_handler_model_factory())
    model_ = Shell::Get()->link_handler_model_factory()->CreateModel(url);
  if (!model_) {
    mojom::LinkHandlerObserverAssociatedPtr observer_ptr;
    observer_ptr.Bind(std::move(observer));
    observer_ptr->OnLinkHandlerModelChanged({});
    return;
  }

  url_ = url;
  model_->SetObserver(std::move(observer));
}

void LinkHandlerController::OpenLinkWithHandler(const GURL& url,
                                                uint32_t handler_id) {
  if (url_ != url)
    return;

  model_->OpenLinkWithHandler(handler_id);
}

}  // namespace ash
