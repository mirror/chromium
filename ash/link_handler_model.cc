// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/link_handler_model.h"

namespace ash {

LinkHandlerModel::LinkHandlerModel() {}
LinkHandlerModel::~LinkHandlerModel() {}

void LinkHandlerModel::SetObserver(
    mojom::LinkHandlerObserverAssociatedPtrInfo observer) {
  observer_.Bind(std::move(observer));
}

}  // namespace ash
