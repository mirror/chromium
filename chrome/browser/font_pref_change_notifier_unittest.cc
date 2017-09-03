// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/font_pref_change_notifier.h"
#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "components/prefs/pref_service_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(FontPrefChangeNotifier, Registrars) {
  // Registrar 0 will outlive the notifier.
  std::vector<std::string> fonts0;
  FontPrefChangeNotifier::Registrar reg0;

  scoped_refptr<PrefRegistry> pref_registry(new PrefRegistry);
  PrefServiceFactory factory;
  std::unique_ptr<PrefService> service = factory.Create(pref_registry.get());

  std::unique_ptr<FontPrefChangeNotifier> notifier =
      base::MakeUnique<FontPrefChangeNotifier>(service.get());
  reg0.Register(notifier.get(),
                base::BindRepeating([&fonts0](const std::string& font) {
                  fonts0.push_back(font);
                }));

  // Registrar 1 will be manually unregistered.
  std::vector<std::string> fonts1;
  FontPrefChangeNotifier::Registrar reg1;
  reg1.Register(notifier.get(),
                base::BindRepeating([&fonts1](const std::string& font) {
                  fonts1.push_back(font);
                }));

  // Registrar 2 will automatically unregister itself when it goes out of scope.
  std::vector<std::string> fonts2;
  {
    FontPrefChangeNotifier::Registrar reg2;
    reg1.Register(notifier.get(),
                  base::BindRepeating([/*&fonts2*/](const std::string& font) {
                    // fonts2.push_back(font);
                  }));

    // All lists should get the font.
    std::string font1("font1");
    notifier->FontChanged(font1);
    EXPECT_EQ(1u, fonts0.size());
    EXPECT_EQ(font1, fonts0.back());
    EXPECT_EQ(1u, fonts1.size());
    EXPECT_EQ(font1, fonts1.back());
    EXPECT_EQ(1u, fonts2.size());
    EXPECT_EQ(font1, fonts2.back());
  }

  // Now that Regsitrar 2 is gone, only 0 and 1 should get changes.
  std::string font2("font2");
  notifier->FontChanged(font2);
  EXPECT_EQ(2u, fonts0.size());
  EXPECT_EQ(font2, fonts0.back());
  EXPECT_EQ(2u, fonts1.size());
  EXPECT_EQ(font2, fonts1.back());
  EXPECT_EQ(1u, fonts2.size());

  // Manually unregister Registrar 1.
  reg1.Unregister();
  EXPECT_FALSE(reg1.is_registered());

  // Only Registrar 0 should see changes now.
  std::string font3("font3");
  notifier->FontChanged(lastfont);
  EXPECT_EQ(3u, fonts0.size());
  EXPECT_EQ(font3, fonts0.back());
  EXPECT_EQ(2u, fonts1.size());
  EXPECT_EQ(1u, fonts2.size());
  EXPECT_EQ(lastfont, fonts0.back());

  notifier.reset();

  // Registrar 0 should have been automatically unregistered.
  EXPECT_FALSE(reg0.is_registered());
}
