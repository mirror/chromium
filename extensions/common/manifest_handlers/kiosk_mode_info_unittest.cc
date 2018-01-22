// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/manifest_handlers/kiosk_mode_info.h"

#include <algorithm>

#include "components/version_info/channel.h"
#include "extensions/common/features/feature_channel.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/manifest_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

using KioskModeInfoManifestTest = ManifestTest;

TEST_F(KioskModeInfoManifestTest, NoSecondaryApps) {
  scoped_refptr<Extension> extension(
      LoadAndExpectSuccess("kiosk_secondary_app_no_secondary_app.json"));
  EXPECT_FALSE(KioskModeInfo::HasSecondaryApps(extension.get()));
}

TEST_F(KioskModeInfoManifestTest, MultipleSecondaryApps) {
  extensions::ScopedCurrentChannel channel(version_info::Channel::DEV);

  const std::vector<std::string> expected_ids_enabled_on_startup = {
      "fiehokkcgaojmbhfhlpiheggjhaedjoc", "ihplaomghjbeafnpnjkhppmfpnmdihgd"};
  const std::vector<std::string> expected_ids_disabled_on_startup = {
      "abcdabcdabcdabcdabcdabcdabcdabcd"};

  scoped_refptr<Extension> extension(
      LoadAndExpectSuccess("kiosk_secondary_app_multi_apps.json"));
  EXPECT_TRUE(KioskModeInfo::HasSecondaryApps(extension.get()));
  KioskModeInfo* info = KioskModeInfo::Get(extension.get());
  EXPECT_NE(nullptr, info);
  std::vector<std::string> ids_enabled_on_startup;
  std::vector<std::string> ids_disabled_on_startup;
  for (const auto& app : info->secondary_apps) {
    if (app.disable_on_startup) {
      ids_disabled_on_startup.push_back(app.id);
    } else {
      ids_enabled_on_startup.push_back(app.id);
    }
  }
  std::sort(ids_enabled_on_startup.begin(), ids_enabled_on_startup.end());
  EXPECT_EQ(expected_ids_enabled_on_startup, ids_enabled_on_startup);
  EXPECT_EQ(expected_ids_disabled_on_startup, ids_disabled_on_startup);
}

TEST_F(KioskModeInfoManifestTest, MultipleSecondaryAppsWithRepeatedEntries) {
  extensions::ScopedCurrentChannel channel(version_info::Channel::DEV);

  const std::vector<std::string> expected_ids_disabled_on_startup = {
      "fiehokkcgaojmbhfhlpiheggjhaedjoc"};
  const std::vector<std::string> expected_ids_enabled_on_startup = {
      "ihplaomghjbeafnpnjkhppmfpnmdihgd"};

  scoped_refptr<Extension> extension(LoadAndExpectSuccess(
      "kiosk_secondary_app_multi_apps_repeated_entries.json"));
  EXPECT_TRUE(KioskModeInfo::HasSecondaryApps(extension.get()));
  KioskModeInfo* info = KioskModeInfo::Get(extension.get());
  EXPECT_NE(nullptr, info);
  std::vector<std::string> ids_enabled_on_startup;
  std::vector<std::string> ids_disabled_on_startup;
  for (const auto& app : info->secondary_apps) {
    if (app.disable_on_startup) {
      ids_disabled_on_startup.push_back(app.id);
    } else {
      ids_enabled_on_startup.push_back(app.id);
    }
  }
  std::sort(ids_enabled_on_startup.begin(), ids_enabled_on_startup.end());
  EXPECT_EQ(expected_ids_enabled_on_startup, ids_enabled_on_startup);
  EXPECT_EQ(expected_ids_disabled_on_startup, ids_disabled_on_startup);
}

TEST_F(KioskModeInfoManifestTest, MultipleSecondaryApps_StableChannel) {
  extensions::ScopedCurrentChannel channel(version_info::Channel::STABLE);

  // The entries that have disable_on_strtup should be ignored on stable
  // channel, so only a single app is expected to be reported.
  const std::vector<std::string> expected_ids_enabled_on_startup = {
      "ihplaomghjbeafnpnjkhppmfpnmdihgd"};
  const std::vector<std::string> expected_ids_disabled_on_startup;

  scoped_refptr<Extension> extension(
      LoadAndExpectSuccess("kiosk_secondary_app_multi_apps.json"));
  EXPECT_TRUE(KioskModeInfo::HasSecondaryApps(extension.get()));
  KioskModeInfo* info = KioskModeInfo::Get(extension.get());
  EXPECT_NE(nullptr, info);
  std::vector<std::string> ids_enabled_on_startup;
  std::vector<std::string> ids_disabled_on_startup;
  for (const auto& app : info->secondary_apps) {
    if (app.disable_on_startup) {
      ids_disabled_on_startup.push_back(app.id);
    } else {
      ids_enabled_on_startup.push_back(app.id);
    }
  }
  std::sort(ids_enabled_on_startup.begin(), ids_enabled_on_startup.end());
  EXPECT_EQ(expected_ids_enabled_on_startup, ids_enabled_on_startup);
  EXPECT_EQ(expected_ids_disabled_on_startup, ids_disabled_on_startup);
}

TEST_F(KioskModeInfoManifestTest,
       MultipleSecondaryAppsWithRepeatedEntries_StableChannel) {
  extensions::ScopedCurrentChannel channel(version_info::Channel::STABLE);

  // The entries that have disable_on_strtup should be ignored on stable
  // channel, so both apps should be reported as enabled on startup.
  const std::vector<std::string> expected_ids_enabled_on_startup = {
      "fiehokkcgaojmbhfhlpiheggjhaedjoc", "ihplaomghjbeafnpnjkhppmfpnmdihgd"};
  const std::vector<std::string> expected_ids_disabled_on_startup;

  scoped_refptr<Extension> extension(LoadAndExpectSuccess(
      "kiosk_secondary_app_multi_apps_repeated_entries.json"));
  EXPECT_TRUE(KioskModeInfo::HasSecondaryApps(extension.get()));
  KioskModeInfo* info = KioskModeInfo::Get(extension.get());
  EXPECT_NE(nullptr, info);
  std::vector<std::string> ids_enabled_on_startup;
  std::vector<std::string> ids_disabled_on_startup;
  for (const auto& app : info->secondary_apps) {
    if (app.disable_on_startup) {
      ids_disabled_on_startup.push_back(app.id);
    } else {
      ids_enabled_on_startup.push_back(app.id);
    }
  }
  std::sort(ids_enabled_on_startup.begin(), ids_enabled_on_startup.end());
  EXPECT_EQ(expected_ids_enabled_on_startup, ids_enabled_on_startup);
  EXPECT_EQ(expected_ids_disabled_on_startup, ids_disabled_on_startup);
}

TEST_F(KioskModeInfoManifestTest,
       RequiredPlatformVersionAndAlwaysUpdateAreOptional) {
  scoped_refptr<Extension> extension(
      LoadAndExpectSuccess("kiosk_required_platform_version_not_present.json"));
  KioskModeInfo* info = KioskModeInfo::Get(extension.get());
  EXPECT_TRUE(info->required_platform_version.empty());
  EXPECT_FALSE(info->always_update);
}

TEST_F(KioskModeInfoManifestTest, RequiredPlatformVersion) {
  scoped_refptr<Extension> extension(
      LoadAndExpectSuccess("kiosk_required_platform_version.json"));
  KioskModeInfo* info = KioskModeInfo::Get(extension.get());
  EXPECT_EQ("1234.0.0", info->required_platform_version);
}

TEST_F(KioskModeInfoManifestTest, RequiredPlatformVersionInvalid) {
  LoadAndExpectError("kiosk_required_platform_version_empty.json",
                     manifest_errors::kInvalidKioskRequiredPlatformVersion);
  LoadAndExpectError("kiosk_required_platform_version_invalid.json",
                     manifest_errors::kInvalidKioskRequiredPlatformVersion);
}

TEST_F(KioskModeInfoManifestTest, AlwaysUpdate) {
  scoped_refptr<Extension> extension(
      LoadAndExpectSuccess("kiosk_always_update.json"));
  KioskModeInfo* info = KioskModeInfo::Get(extension.get());
  EXPECT_TRUE(info->always_update);
}

TEST_F(KioskModeInfoManifestTest, AlwaysUpdateFalse) {
  scoped_refptr<Extension> extension(
      LoadAndExpectSuccess("kiosk_always_update_false.json"));
  KioskModeInfo* info = KioskModeInfo::Get(extension.get());
  EXPECT_FALSE(info->always_update);
}

TEST_F(KioskModeInfoManifestTest, AlwaysUpdateInvalid) {
  LoadAndExpectError("kiosk_always_update_invalid.json",
                     manifest_errors::kInvalidKioskAlwaysUpdate);
}

}  // namespace extensions
