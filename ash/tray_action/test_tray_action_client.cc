// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/tray_action/test_tray_action_client.h"

namespace ash {

TestTrayActionClient::TestTrayActionClient() : binding_(this) {}

TestTrayActionClient::~TestTrayActionClient() = default;

mojom::TrayActionClientPtr TestTrayActionClient::CreateInterfacePtrAndBind() {
  mojom::TrayActionClientPtr ptr;
  binding_.Bind(mojo::MakeRequest(&ptr));
  return ptr;
}

void TestTrayActionClient::RequestNewLockScreenNote() {
  action_requests_count_++;
}

}  // namespace ash