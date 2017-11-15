// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/seat.h"

#include "base/memory/scoped_refptr.h"
#include "components/exo/keyboard.h"
#include "components/exo/test/exo_test_base.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace exo {
namespace {

using SeatTest = test::ExoTestBase;

class MockKeyboard : public Keyboard {
 public:
  MockKeyboard(Seat* seat) : Keyboard(nullptr, seat){};

  unsigned int on_window_focused_count() { return on_window_focused_count_; }

  // Overridden from Keyboard:
  void OnSurfaceFocused(Surface* surface) override {
    on_window_focused_count_++;
  }

 private:
  unsigned int on_window_focused_count_ = 0;
};

TEST_F(SeatTest, Basic) {
  Seat seat;
  auto keyboard = std::make_unique<MockKeyboard>(&seat);

  seat.OnWindowFocused(nullptr, nullptr);
  ASSERT_EQ(1u, keyboard->on_window_focused_count());

  // OnWindowFocused can be invoked after keyboard is released.
  keyboard.reset();
  seat.OnWindowFocused(nullptr, nullptr);
}

}  // namespace
}  // namespace exo
