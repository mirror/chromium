// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/link_handler_model_factory.h"

#include "ash/link_handler_model.h"

namespace ash {

LinkHandlerModelFactory::LinkHandlerModelFactory() {}
LinkHandlerModelFactory::~LinkHandlerModelFactory() {}

void LinkHandlerModelFactory::BindRequest(
    mojom::LinkHandlerControllerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

std::unique_ptr<LinkHandlerModel> LinkHandlerModelFactory::CreateModel(
    const GURL& url) {
  return nullptr;
}

void LinkHandlerModelFactory::ConnectModel(
    mojom::LinkHandlerObserverAssociatedPtrInfo observer,
    const GURL& url) {
  auto model = CreateModel(url);
  model->AddObserver(std::move(observer));
  // FIXME add model to map
}

void LinkHandlerModelFactory::OpenLinkWithHandler(const GURL& url,
                                                  uint32_t handler_id) {
  // FIXME
  // map[url].OpenLinkWithHandler(handler_id);
}

}  // namespace ash
