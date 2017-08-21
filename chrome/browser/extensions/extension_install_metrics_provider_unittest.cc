// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_install_metrics_provider.h"

#include <algorithm>

#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_service_test_base.h"
#include "chrome/browser/profiles/profile.h"
#include "components/metrics/proto/extension_install.pb.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/common/extension_builder.h"
#include "testing/gtest/include/gtest/gtest.h"

using metrics::ExtensionInstallProto;

namespace extensions {

class ExtensionInstallMetricsProviderTest : public ExtensionServiceTestBase {
 public:
  ExtensionInstallMetricsProviderTest() {}
  ~ExtensionInstallMetricsProviderTest() override {}

  void SetUp() override {
    ExtensionServiceTestBase::SetUp();
    InitializeEmptyExtensionService();
    prefs_ = ExtensionPrefs::Get(profile());
  }

  // Wrappers around ExtensionInstallMetricsProvider methods since the test
  // class is friended.
  ExtensionInstallProto ConstructProto(const Extension& extension) {
    return ExtensionInstallMetricsProvider().ConstructProto(extension, prefs_);
  }
  std::vector<ExtensionInstallProto> GetInstallsForProfile() {
    return ExtensionInstallMetricsProvider().GetInstallsForProfile(profile());
  }

  ExtensionPrefs* prefs() { return prefs_; }

 private:
  ExtensionPrefs* prefs_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(ExtensionInstallMetricsProviderTest);
};

// Tests the various aspects of constructing a relevant proto for a given
// extension installation.
TEST_F(ExtensionInstallMetricsProviderTest, TestProtoConstruction) {
  {
    // Test basic prototype construction. All fields should be present, except
    // disable reasons (which should be empty).
    scoped_refptr<const Extension> extension =
        ExtensionBuilder("test").SetLocation(Manifest::INTERNAL).Build();
    ExtensionInstallProto install = ConstructProto(*extension);
    EXPECT_TRUE(install.has_type());
    EXPECT_EQ(ExtensionInstallProto::EXTENSION, install.type());

    EXPECT_TRUE(install.has_install_location());
    EXPECT_EQ(ExtensionInstallProto::INTERNAL, install.install_location());

    EXPECT_TRUE(install.has_manifest_version());
    EXPECT_EQ(2, install.manifest_version());

    EXPECT_TRUE(install.has_action_type());
    EXPECT_EQ(ExtensionInstallProto::NO_ACTION, install.action_type());

    EXPECT_TRUE(install.has_has_file_access());
    EXPECT_FALSE(install.has_file_access());

    EXPECT_TRUE(install.has_has_incognito_access());
    EXPECT_FALSE(install.has_incognito_access());

    EXPECT_TRUE(install.has_updates_from_store());
    EXPECT_FALSE(install.updates_from_store());

    EXPECT_TRUE(install.has_is_from_bookmark());
    EXPECT_FALSE(install.is_from_bookmark());

    EXPECT_TRUE(install.has_is_converted_from_user_script());
    EXPECT_FALSE(install.is_converted_from_user_script());

    EXPECT_TRUE(install.has_is_default_installed());
    EXPECT_FALSE(install.is_default_installed());

    EXPECT_TRUE(install.has_is_oem_installed());
    EXPECT_FALSE(install.is_oem_installed());

    EXPECT_TRUE(install.has_background_script_type());
    EXPECT_EQ(ExtensionInstallProto::NO_BACKGROUND_SCRIPT,
              install.background_script_type());

    EXPECT_EQ(0, install.disable_reasons_size());

    EXPECT_TRUE(install.has_blacklist_state());
    EXPECT_EQ(ExtensionInstallProto::NOT_BLACKLISTED,
              install.blacklist_state());
  }

  // It's not helpful to exhaustively test each possible variation of each
  // field in the proto (since in many cases the test code would then be
  // re-writing the original code), but we test a few of the more interesting
  // cases.

  {
    // Test the type() field; extensions of different types should be reported
    // as such.
    scoped_refptr<const Extension> extension =
        ExtensionBuilder("app", ExtensionBuilder::Type::PLATFORM_APP)
            .SetLocation(Manifest::INTERNAL)
            .Build();
    ExtensionInstallProto install = ConstructProto(*extension);
    EXPECT_EQ(ExtensionInstallProto::PLATFORM_APP, install.type());
  }

  {
    // Test the install location.
    scoped_refptr<const Extension> extension =
        ExtensionBuilder("unpacked").SetLocation(Manifest::UNPACKED).Build();
    ExtensionInstallProto install = ConstructProto(*extension);
    EXPECT_EQ(ExtensionInstallProto::UNPACKED, install.install_location());
  }

  {
    // Test the extension action as a browser action.
    scoped_refptr<const Extension> extension =
        ExtensionBuilder("browser_action")
            .SetLocation(Manifest::INTERNAL)
            .SetAction(ExtensionBuilder::ActionType::BROWSER_ACTION)
            .Build();
    ExtensionInstallProto install = ConstructProto(*extension);
    EXPECT_EQ(ExtensionInstallProto::BROWSER_ACTION, install.action_type());
  }

  {
    // Test the extension action as a page action.
    scoped_refptr<const Extension> extension =
        ExtensionBuilder("page_action")
            .SetLocation(Manifest::INTERNAL)
            .SetAction(ExtensionBuilder::ActionType::PAGE_ACTION)
            .Build();
    ExtensionInstallProto install = ConstructProto(*extension);
    EXPECT_EQ(ExtensionInstallProto::PAGE_ACTION, install.action_type());
  }

  {
    // Test the disable reasons field.
    scoped_refptr<const Extension> extension =
        ExtensionBuilder("disable_reasons")
            .SetLocation(Manifest::INTERNAL)
            .Build();
    prefs()->AddDisableReason(extension->id(), Extension::DISABLE_USER_ACTION);
    {
      ExtensionInstallProto install = ConstructProto(*extension);
      ASSERT_EQ(1, install.disable_reasons_size());
      EXPECT_EQ(ExtensionInstallProto::USER_ACTION,
                install.disable_reasons().Get(0));
    }
    // Adding additional disable reasons should result in all reasons being
    // reported.
    prefs()->AddDisableReason(extension->id(), Extension::DISABLE_CORRUPTED);
    {
      ExtensionInstallProto install = ConstructProto(*extension);
      ASSERT_EQ(2, install.disable_reasons_size());
      EXPECT_EQ(ExtensionInstallProto::USER_ACTION,
                install.disable_reasons().Get(0));
      EXPECT_EQ(ExtensionInstallProto::CORRUPTED,
                install.disable_reasons().Get(1));
    }
  }

  {
    // Test that event pages are reported correctly.
    DictionaryBuilder background;
    background.SetBoolean("persistent", false)
        .Set("scripts", ListBuilder().Append("script.js").Build());
    scoped_refptr<const Extension> extension =
        ExtensionBuilder("event_page")
            .SetLocation(Manifest::INTERNAL)
            .MergeManifest(DictionaryBuilder()
                               .Set("background", background.Build())
                               .Build())
            .Build();
    ExtensionInstallProto install = ConstructProto(*extension);
    EXPECT_EQ(ExtensionInstallProto::EVENT_PAGE,
              install.background_script_type());
  }

  {
    // Test that persistent background pages are reported correctly.
    DictionaryBuilder background;
    background.SetBoolean("persistent", true)
        .Set("scripts", ListBuilder().Append("script.js").Build());
    scoped_refptr<const Extension> extension =
        ExtensionBuilder("persisent_background")
            .SetLocation(Manifest::INTERNAL)
            .MergeManifest(DictionaryBuilder()
                               .Set("background", background.Build())
                               .Build())
            .Build();
    ExtensionInstallProto install = ConstructProto(*extension);
    EXPECT_EQ(ExtensionInstallProto::PERSISTENT_BACKGROUND_PAGE,
              install.background_script_type());
  }

  {
    // Test changing the blacklist state.
    scoped_refptr<const Extension> extension =
        ExtensionBuilder("blacklist").SetLocation(Manifest::INTERNAL).Build();
    prefs()->SetExtensionBlacklistState(extension->id(),
                                        BLACKLISTED_SECURITY_VULNERABILITY);
    ExtensionInstallProto install = ConstructProto(*extension);
    EXPECT_EQ(ExtensionInstallProto::BLACKLISTED_SECURITY_VULNERABILITY,
              install.blacklist_state());
  }
}

// Tests that we retrieve all extensions associated with a given profile.
TEST_F(ExtensionInstallMetricsProviderTest, TestGettingAllExtensionsInProfile) {
  scoped_refptr<const Extension> extension =
      ExtensionBuilder("extension").Build();
  service()->AddExtension(extension.get());
  scoped_refptr<const Extension> app =
      ExtensionBuilder("app", ExtensionBuilder::Type::PLATFORM_APP).Build();
  service()->AddExtension(app.get());
  service()->DisableExtension(app->id(), Extension::DISABLE_USER_ACTION);

  std::vector<ExtensionInstallProto> installs = GetInstallsForProfile();
  // There should be two installs total.
  ASSERT_EQ(2u, installs.size());
  // One should be the extension, and the other should be the app. We don't
  // check the specifics of the proto, since that's tested above.
  EXPECT_TRUE(std::any_of(installs.begin(), installs.end(),
                          [](const ExtensionInstallProto& install) {
                            return install.type() ==
                                   ExtensionInstallProto::EXTENSION;
                          }));
  EXPECT_TRUE(std::any_of(installs.begin(), installs.end(),
                          [](const ExtensionInstallProto& install) {
                            return install.type() ==
                                   ExtensionInstallProto::EXTENSION;
                          }));
}

}  // namespace extensions
