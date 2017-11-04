// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_TEST_FAKE_POWER_INSTANCE_H_
#define COMPONENTS_ARC_TEST_FAKE_POWER_INSTANCE_H_

#include "base/macros.h"
#include "components/arc/common/power.mojom.h"

namespace arc {

class FakePowerInstance : public mojom::PowerInstance {
 public:
  FakePowerInstance();
  ~FakePowerInstance() override;

  // mojom::PowerInstance overrides:
  void Init(mojom::PowerHostPtr host_ptr) override;
  void SetInteractive(bool enabled) override;
  void Suspend(SuspendCallback callback) override;
  void Resume() override;
  void UpdateScreenBrightnessSettings(double percent) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(FakePowerInstance);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_TEST_FAKE_POWER_INSTANCE_H_
