// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_TRAY_ACTION_TEST_TRAY_ACTION_CLIENT_H_
#define ASH_TRAY_ACTION_TEST_TRAY_ACTION_CLIENT_H_

#include "ash/public/interfaces/tray_action.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace ash {

class TestTrayActionClient : public mojom::TrayActionClient {
 public:
  TestTrayActionClient();

  ~TestTrayActionClient() override;

  mojom::TrayActionClientPtr CreateInterfacePtrAndBind();

  // mojom::TrayActionClient:
  void RequestNewLockScreenNote() override;
  MOCK_METHOD1(ToggleForegroundMode, void(bool move_to_foreground));

  int action_requests_count() const { return action_requests_count_; }

 private:
  mojo::Binding<ash::mojom::TrayActionClient> binding_;

  int action_requests_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(TestTrayActionClient);
};

}  // namespace ash

#endif  // ASH_TRAY_ACTION_TEST_TRAY_ACTION_CLIENT_H_