// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/test/mock_display_private_client.h"

namespace viz {

MockDisplayPrivateClient::MockDisplayPrivateClient() : binding_(this) {}

MockDisplayPrivateClient::~MockDisplayPrivateClient() = default;

mojom::DisplayPrivateClientPtr MockDisplayPrivateClient::BindInterfacePtr() {
  mojom::DisplayPrivateClientPtr ptr;
  binding_.Bind(MakeRequest(&ptr));
  return ptr;
}

}  // namespace viz
