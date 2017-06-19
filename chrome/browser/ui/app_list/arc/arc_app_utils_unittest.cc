// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/arc/arc_app_utils.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"
#include "chrome/browser/ui/ash/launcher/arc_app_shelf_id.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr char kTestPackage[] = "com.test.app";
constexpr char kTestActivity[] = "com.test.app.some.activity";
constexpr char kTestShelfGroupId[] = "some_shelf_group";
constexpr char kIntentWithShelfGroupId[] =
    "#Intent;launchFlags=0x18001000;"
    "component=com.test.app/com.test.app.some.activity;"
    "S.org.chromium.arc.shelf_group_id=some_shelf_group;end";

std::string GetPlayStoreInitialLaunchIntent() {
  std::vector<std::string> extra_params;
  extra_params.push_back(arc::kInitialStartParam);
  return arc::GetLaunchIntent(arc::kPlayStorePackage, arc::kPlayStoreActivity,
                              extra_params);
}

}  // namespace

class ArcAppUtilsTest : public testing::Test {
 public:
  ArcAppUtilsTest() = default;
  ~ArcAppUtilsTest() override = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(ArcAppUtilsTest);
};

TEST_F(ArcAppUtilsTest, LaunchIntent) {
  const std::string launch_intent = GetPlayStoreInitialLaunchIntent();

  std::string action;
  std::string category;
  std::string package_name;
  std::string activity;
  int launch_flags = 0;
  std::vector<std::string> extra_params;
  EXPECT_TRUE(arc::ParseIntent(launch_intent, &action, &category, &package_name,
                               &activity, &launch_flags, &extra_params));
  EXPECT_EQ(action, "android.intent.action.MAIN");
  EXPECT_EQ(category, "android.intent.category.LAUNCHER");
  EXPECT_EQ(package_name, arc::kPlayStorePackage);
  EXPECT_EQ(activity, arc::kPlayStoreActivity);
  // FLAG_ACTIVITY_NEW_TASK + FLAG_ACTIVITY_RESET_TASK_IF_NEEDED.
  EXPECT_EQ(launch_flags, 0x10200000);
  ASSERT_EQ(extra_params.size(), 1U);
  EXPECT_EQ(extra_params[0], arc::kInitialStartParam);

  action.clear();
  category.clear();
  package_name.clear();
  activity.clear();
  launch_flags = 0;
  extra_params.clear();
  EXPECT_TRUE(arc::ParseIntent(kIntentWithShelfGroupId, &action, &category,
                               &package_name, &activity, &launch_flags,
                               &extra_params));
  EXPECT_EQ(action, "");
  EXPECT_EQ(category, "");
  EXPECT_EQ(package_name, kTestPackage);
  EXPECT_EQ(activity, kTestActivity);
  EXPECT_EQ(launch_flags, 0x18001000);
  ASSERT_EQ(extra_params.size(), 1U);
  EXPECT_EQ(extra_params[0],
            "S.org.chromium.arc.shelf_group_id=some_shelf_group");
}

TEST_F(ArcAppUtilsTest, ShelfGroupId) {
  const std::string intent_with_shelf_group_id(kIntentWithShelfGroupId);
  const std::string shelf_app_id =
      ArcAppListPrefs::GetAppId(kTestPackage, kTestActivity);
  arc::ArcAppShelfId shelf_id1 = arc::ArcAppShelfId::FromIntentAndAppId(
      intent_with_shelf_group_id, shelf_app_id);
  EXPECT_TRUE(shelf_id1.has_shelf_group_id());
  EXPECT_EQ(shelf_id1.shelf_group_id(), kTestShelfGroupId);
  EXPECT_EQ(shelf_id1.app_id(), shelf_app_id);

  arc::ArcAppShelfId shelf_id2 = arc::ArcAppShelfId::FromIntentAndAppId(
      GetPlayStoreInitialLaunchIntent(), arc::kPlayStoreAppId);
  EXPECT_FALSE(shelf_id2.has_shelf_group_id());
  EXPECT_EQ(shelf_id2.app_id(), arc::kPlayStoreAppId);
}
