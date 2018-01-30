// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/display/persistent_window_controller.h"

#include "ash/public/cpp/ash_switches.h"
#include "ash/test/ash_test_base.h"
#include "base/command_line.h"

namespace ash {

class PersistentWindowControllerTest : public AshTestBase {
 protected:
  PersistentWindowControllerTest() = default;
  ~PersistentWindowControllerTest() override = default;

  // AshTestBase:
  void SetUp() override {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kAshEnablePersistentWindowBounds);
    AshTestBase::SetUp();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(PersistentWindowControllerTest);
};

TEST_F(PersistentWindowControllerTest, DisconnectExternal) {
  UpdateDisplay("0+0-500x500,0+501-500x500");
  UpdateDisplay("0+0-500x500");
}

}  // namespace ash
