// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/os_crypt/key_storage_util_linux.h"
#include "base/logging.h"
#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

using namespace os_crypt;

class KeyStorageUtilLinuxTest : public testing::Test {
 public:
  KeyStorageUtilLinuxTest() = default;
  ~KeyStorageUtilLinuxTest() override = default;

  void SetUp() override {}

  void TearDown() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(KeyStorageUtilLinuxTest);
};

TEST_F(KeyStorageUtilLinuxTest, PasswordStoreFlagOverrides) {
  SelectedLinuxBackend selected;

  selected = SelectBackend(
      "basic", true, base::nix::DesktopEnvironment::DESKTOP_ENVIRONMENT_GNOME);
  EXPECT_EQ(selected, SelectedLinuxBackend::BASIC_TEXT);

  selected =
      SelectBackend("gnome-libsecret", false,
                    base::nix::DesktopEnvironment::DESKTOP_ENVIRONMENT_KDE4);
  EXPECT_EQ(selected, SelectedLinuxBackend::GNOME_LIBSECRET);

  selected =
      SelectBackend("gnome-libsecret", true,
                    base::nix::DesktopEnvironment::DESKTOP_ENVIRONMENT_KDE4);
  EXPECT_EQ(selected, SelectedLinuxBackend::GNOME_LIBSECRET);
}

TEST_F(KeyStorageUtilLinuxTest, IgnoreBackends) {
  SelectedLinuxBackend selected;

  selected = SelectBackend(
      "", true, base::nix::DesktopEnvironment::DESKTOP_ENVIRONMENT_GNOME);
  EXPECT_EQ(selected, SelectedLinuxBackend::GNOME_ANY);

  selected = SelectBackend(
      "", false, base::nix::DesktopEnvironment::DESKTOP_ENVIRONMENT_GNOME);
  EXPECT_EQ(selected, SelectedLinuxBackend::BASIC_TEXT);

  selected = SelectBackend(
      "", true, base::nix::DesktopEnvironment::DESKTOP_ENVIRONMENT_KDE5);
  EXPECT_EQ(selected, SelectedLinuxBackend::KWALLET5);

  selected = SelectBackend(
      "", false, base::nix::DesktopEnvironment::DESKTOP_ENVIRONMENT_KDE5);
  EXPECT_EQ(selected, SelectedLinuxBackend::BASIC_TEXT);
}

}  // namespace
