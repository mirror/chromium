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

TEST_F(ExtensionInstallMetricsProviderTest, TestProtoConstruction) {
  {
    scoped_refptr<const Extension> extension =
        ExtensionBuilder("test").SetLocation(Manifest::INTERNAL).Build();
    ExtensionInstallProto install = ConstructProto(*extension);
    EXPECT_EQ(ExtensionInstallProto::EXTENSION, install.type());
    EXPECT_EQ(ExtensionInstallProto::INTERNAL, install.install_location());
    EXPECT_EQ(2, install.manifest_version());
    EXPECT_EQ(ExtensionInstallProto::NO_ACTION, install.action_type());
    EXPECT_FALSE(install.has_file_access());
    EXPECT_FALSE(install.has_incognito_access());
    EXPECT_FALSE(install.updates_from_store());
    EXPECT_FALSE(install.is_from_bookmark());
    EXPECT_FALSE(install.is_converted_from_user_script());
    EXPECT_FALSE(install.is_default_installed());
    EXPECT_FALSE(install.is_oem_installed());
    EXPECT_EQ(ExtensionInstallProto::NO_BACKGROUND_SCRIPT,
              install.background_script_type());
    EXPECT_EQ(0, install.disable_reasons_size());
    EXPECT_EQ(ExtensionInstallProto::NOT_BLACKLISTED,
              install.blacklist_state());
  }

  {
    scoped_refptr<const Extension> extension =
        ExtensionBuilder("app", ExtensionBuilder::Type::PLATFORM_APP)
            .SetLocation(Manifest::INTERNAL)
            .Build();
    ExtensionInstallProto install = ConstructProto(*extension);
    EXPECT_EQ(ExtensionInstallProto::PLATFORM_APP, install.type());
  }

  {
    scoped_refptr<const Extension> extension =
        ExtensionBuilder("unpacked").SetLocation(Manifest::UNPACKED).Build();
    ExtensionInstallProto install = ConstructProto(*extension);
    EXPECT_EQ(ExtensionInstallProto::UNPACKED, install.install_location());
  }

  {
    scoped_refptr<const Extension> extension =
        ExtensionBuilder("browser_action")
            .SetLocation(Manifest::INTERNAL)
            .SetAction(ExtensionBuilder::ActionType::BROWSER_ACTION)
            .Build();
    ExtensionInstallProto install = ConstructProto(*extension);
    EXPECT_EQ(ExtensionInstallProto::BROWSER_ACTION, install.action_type());
  }

  {
    scoped_refptr<const Extension> extension =
        ExtensionBuilder("page_action")
            .SetLocation(Manifest::INTERNAL)
            .SetAction(ExtensionBuilder::ActionType::PAGE_ACTION)
            .Build();
    ExtensionInstallProto install = ConstructProto(*extension);
    EXPECT_EQ(ExtensionInstallProto::PAGE_ACTION, install.action_type());
  }

  {
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
    scoped_refptr<const Extension> extension =
        ExtensionBuilder("blacklist").SetLocation(Manifest::INTERNAL).Build();
    prefs()->SetExtensionBlacklistState(extension->id(),
                                        BLACKLISTED_SECURITY_VULNERABILITY);
    ExtensionInstallProto install = ConstructProto(*extension);
    EXPECT_EQ(ExtensionInstallProto::BLACKLISTED_SECURITY_VULNERABILITY,
              install.blacklist_state());
  }
}

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
