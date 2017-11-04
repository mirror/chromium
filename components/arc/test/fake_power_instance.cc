// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/test/fake_power_instance.h"

namespace arc {

FakePowerInstance::FakePowerInstance() = default;

FakePowerInstance::~FakePowerInstance() = default;

void FakePowerInstance::Init(mojom::PowerHostPtr host_ptr) {}

void FakePowerInstance::SetInteractive(bool enabled) {}

void FakePowerInstance::Suspend(SuspendCallback callback) {}

void FakePowerInstance::Resume() {}

void FakePowerInstance::UpdateScreenBrightnessSettings(double percent) {}

}  // namespace arc
