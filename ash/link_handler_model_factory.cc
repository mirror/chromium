// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/link_handler_model_factory.h"

#include "ash/link_handler_model.h"
#include "ash/shell.h"

#include "components/vector_icons/vector_icons.h"
#include "ui/gfx/paint_vector_icon.h"
#include "base/strings/utf_string_conversions.h"

namespace ash {

LinkHandlerController::LinkHandlerController() {}
LinkHandlerController::~LinkHandlerController() {}

void LinkHandlerController::BindRequest(
    mojom::LinkHandlerControllerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void LinkHandlerController::ConnectModel(
    mojom::LinkHandlerObserverAssociatedPtrInfo observer,
    const GURL& url) {
  model_.reset();
  if (Shell::Get()->link_handler_model_factory())
    model_ = Shell::Get()->link_handler_model_factory()->CreateModel(url);
  if (!model_) {
#if 1
    std::vector<ash::mojom::LinkHandlerInfoPtr> handlers;
    ash::mojom::LinkHandlerInfoPtr handler = ash::mojom::LinkHandlerInfo::New();
    handler->name = "Test link handler";
    handler->icon =
        gfx::CreateVectorIcon(vector_icons::kBusinessIcon, 16, SK_ColorRED);
    handler->icon.EnsureRepsForSupportedScales();
    handler->id = 0;
    handlers.push_back(std::move(handler));
    mojom::LinkHandlerObserverAssociatedPtr observer_ptr;
    observer_ptr.Bind(std::move(observer));
    observer_ptr->OnLinkHandlerModelChanged(std::move(handlers));
#else
    url_ = GURL();
    mojom::LinkHandlerObserverAssociatedPtr observer_ptr;
    observer_ptr.Bind(std::move(observer));
    observer_ptr->OnLinkHandlerModelChanged({});
#endif
    return;
  }

  url_ = url;
  model_->AddObserver(std::move(observer));
}

void LinkHandlerController::OpenLinkWithHandler(const GURL& url,
                                                  uint32_t handler_id) {
  if (url_ != url)
    return;

  model_->OpenLinkWithHandler(handler_id);
}

}  // namespace ash
