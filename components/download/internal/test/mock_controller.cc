// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/test/mock_controller.h"

namespace download {
namespace test {

MockController::MockController() = default;
MockController::~MockController() = default;

void MockController::Initialize(const InitCallback& callback) {
  init_callback_ = callback;
}

void MockController::TriggerInitCompleted(bool startup_ok) {
  if (init_callback_.is_null())
    return;

  init_callback_.Run(startup_ok);
}

}  // namespace test
}  // namespace download
