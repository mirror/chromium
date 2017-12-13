// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/keyboard_shortcut_viewer_controller_struct_traits.h"

#include "ash/public/cpp/keyboard_shortcut_viewer_controller_struct_traits_test_service.mojom.h"
#include "ash/public/interfaces/keyboard_shortcut_viewer_controller.mojom.h"
#include "base/message_loop/message_loop.h"
#include "components/keyboard_shortcut_viewer/keyboard_shortcut_info.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ash {
namespace {

class KeyboardShortcutViewerControllerStructTraitsTest
    : public testing::Test,
      public mojom::KeyboardShortcutViewerControllerStructTraitsTestService {
 public:
  KeyboardShortcutViewerControllerStructTraitsTest() : binding_(this) {}

  // testing::Test:
  void SetUp() override { binding_.Bind(mojo::MakeRequest(&service_)); }

  mojom::KeyboardShortcutViewerControllerStructTraitsTestServicePtr& service() {
    return service_;
  }

 private:
  // mojom::KeyboardShortcutViewerControllerStructTraitsTestService:
  void EchoKeyboardShortcutItemInfo(
      const keyboard_shortcut_viewer::KeyboardShortcutItemInfo& in,
      EchoKeyboardShortcutItemInfoCallback callback) override {
    std::move(callback).Run(in);
  }

  base::MessageLoop loop_;
  mojo::Binding<KeyboardShortcutViewerControllerStructTraitsTestService>
      binding_;
  mojom::KeyboardShortcutViewerControllerStructTraitsTestServicePtr service_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardShortcutViewerControllerStructTraitsTest);
};

TEST_F(KeyboardShortcutViewerControllerStructTraitsTest, Basic) {
  keyboard_shortcut_viewer::KeyboardShortcutItemInfo info_in;
  info_in.categories.push_back(1);
  info_in.categories.push_back(2);
  info_in.description = 2;
  info_in.shortcut = 3;

  keyboard_shortcut_viewer::ShortcutReplacementInfo replacement;
  replacement.replacement_id = static_cast<int>(ui::EF_CONTROL_DOWN);
  replacement.replacement_type = keyboard_shortcut_viewer::MODIFIER;
  info_in.shortcut_replacements_info.push_back(replacement);

  replacement.replacement_id = static_cast<int>(ui::VKEY_L);
  replacement.replacement_type = keyboard_shortcut_viewer::VKEY;
  info_in.shortcut_replacements_info.push_back(replacement);

  replacement.replacement_id = static_cast<int>(ui::VKEY_L);
  replacement.replacement_type = keyboard_shortcut_viewer::ICON;
  info_in.shortcut_replacements_info.push_back(replacement);

  keyboard_shortcut_viewer::KeyboardShortcutItemInfo info_out;
  service()->EchoKeyboardShortcutItemInfo(info_in, &info_out);
  EXPECT_EQ(info_in, info_out);
}

}  // namespace
}  // namespace ash
