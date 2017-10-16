// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/intent_helper/arc_intent_helper_bridge.h"

#include <memory>
#include <utility>

#include "ash/shell.h"
#include "ash/shell_init_params.h"
#include "ash/test/ash_test_base.h"
#include "ash/test/ash_test_environment.h"
#include "ash/test/ash_test_helper.h"
#include "ash/test_shell_delegate.h"
#include "base/test/test_simple_task_runner.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/common/intent_helper.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/message_center/message_center.h"

namespace arc {

namespace {

IntentFilter GetIntentFilter(const std::string& host) {
  std::vector<IntentFilter::AuthorityEntry> authorities;
  authorities.emplace_back(host, -1);
  return IntentFilter(std::move(authorities),
                      std::vector<IntentFilter::PatternMatcher>());
}

class ArcIntentHelperTest : public testing::Test {
 public:
  ArcIntentHelperTest() = default;

 protected:
  std::unique_ptr<ArcBridgeService> arc_bridge_service_;
  std::unique_ptr<ash::TestShellDelegate> test_shell_delegate_;
  std::unique_ptr<ArcIntentHelperBridge> instance_;

 private:
  void SetUp() override {
    message_center::MessageCenter::Initialize();
    chromeos::DBusThreadManager::Initialize();
    arc_bridge_service_ = std::make_unique<ArcBridgeService>();
    instance_ = std::make_unique<ArcIntentHelperBridge>(
        nullptr /* context */, arc_bridge_service_.get());
    test_task_runner_ = base::MakeRefCounted<base::TestSimpleTaskRunner>();

    test_shell_delegate_ = std::make_unique<ash::TestShellDelegate>();
    ash::ShellInitParams init_params;
    init_params.delegate = test_shell_delegate_.get();
    ash::Shell::CreateInstance(init_params);
    // ash_test_helper()->set_test_shell_delegate(test_shell_delegate_.get());
  }

  void TearDown() override {
    instance_.reset();
    arc_bridge_service_.reset();
    chromeos::DBusThreadManager::Shutdown();
    // Ash Shell can't just live on its own without a browser process, we need
    // to also shut down the message center.
    message_center::MessageCenter::Shutdown();
    ash::Shell::DeleteInstance();
  }

  scoped_refptr<base::TestSimpleTaskRunner> test_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(ArcIntentHelperTest);
};

}  // namespace

// Tests if IsIntentHelperPackage works as expected. Probably too trivial
// to test but just in case.
TEST_F(ArcIntentHelperTest, TestIsIntentHelperPackage) {
  EXPECT_FALSE(ArcIntentHelperBridge::IsIntentHelperPackage(""));
  EXPECT_FALSE(ArcIntentHelperBridge::IsIntentHelperPackage(
      ArcIntentHelperBridge::kArcIntentHelperPackageName + std::string("a")));
  EXPECT_FALSE(ArcIntentHelperBridge::IsIntentHelperPackage(
      ArcIntentHelperBridge::kArcIntentHelperPackageName +
      std::string("/.ArcIntentHelperActivity")));
  EXPECT_TRUE(ArcIntentHelperBridge::IsIntentHelperPackage(
      ArcIntentHelperBridge::kArcIntentHelperPackageName));
}

// Tests if FilterOutIntentHelper removes handlers as expected.
TEST_F(ArcIntentHelperTest, TestFilterOutIntentHelper) {
  {
    std::vector<mojom::IntentHandlerInfoPtr> orig;
    std::vector<mojom::IntentHandlerInfoPtr> filtered =
        ArcIntentHelperBridge::FilterOutIntentHelper(std::move(orig));
    EXPECT_EQ(0U, filtered.size());
  }

  {
    std::vector<mojom::IntentHandlerInfoPtr> orig;
    orig.push_back(mojom::IntentHandlerInfo::New());
    orig[0]->name = "0";
    orig[0]->package_name = "package_name0";
    orig.push_back(mojom::IntentHandlerInfo::New());
    orig[1]->name = "1";
    orig[1]->package_name = "package_name1";

    // FilterOutIntentHelper is no-op in this case.
    std::vector<mojom::IntentHandlerInfoPtr> filtered =
        ArcIntentHelperBridge::FilterOutIntentHelper(std::move(orig));
    EXPECT_EQ(2U, filtered.size());
  }

  {
    std::vector<mojom::IntentHandlerInfoPtr> orig;
    orig.push_back(mojom::IntentHandlerInfo::New());
    orig[0]->name = "0";
    orig[0]->package_name = ArcIntentHelperBridge::kArcIntentHelperPackageName;
    orig.push_back(mojom::IntentHandlerInfo::New());
    orig[1]->name = "1";
    orig[1]->package_name = "package_name1";

    // FilterOutIntentHelper should remove the first element.
    std::vector<mojom::IntentHandlerInfoPtr> filtered =
        ArcIntentHelperBridge::FilterOutIntentHelper(std::move(orig));
    ASSERT_EQ(1U, filtered.size());
    EXPECT_EQ("1", filtered[0]->name);
    EXPECT_EQ("package_name1", filtered[0]->package_name);
  }

  {
    std::vector<mojom::IntentHandlerInfoPtr> orig;
    orig.push_back(mojom::IntentHandlerInfo::New());
    orig[0]->name = "0";
    orig[0]->package_name = ArcIntentHelperBridge::kArcIntentHelperPackageName;
    orig.push_back(mojom::IntentHandlerInfo::New());
    orig[1]->name = "1";
    orig[1]->package_name = "package_name1";
    orig.push_back(mojom::IntentHandlerInfo::New());
    orig[2]->name = "2";
    orig[2]->package_name = ArcIntentHelperBridge::kArcIntentHelperPackageName;

    // FilterOutIntentHelper should remove two elements.
    std::vector<mojom::IntentHandlerInfoPtr> filtered =
        ArcIntentHelperBridge::FilterOutIntentHelper(std::move(orig));
    ASSERT_EQ(1U, filtered.size());
    EXPECT_EQ("1", filtered[0]->name);
    EXPECT_EQ("package_name1", filtered[0]->package_name);
  }

  {
    std::vector<mojom::IntentHandlerInfoPtr> orig;
    orig.push_back(mojom::IntentHandlerInfo::New());
    orig[0]->name = "0";
    orig[0]->package_name = ArcIntentHelperBridge::kArcIntentHelperPackageName;
    orig.push_back(mojom::IntentHandlerInfo::New());
    orig[1]->name = "1";
    orig[1]->package_name = ArcIntentHelperBridge::kArcIntentHelperPackageName;

    // FilterOutIntentHelper should remove all elements.
    std::vector<mojom::IntentHandlerInfoPtr> filtered =
        ArcIntentHelperBridge::FilterOutIntentHelper(std::move(orig));
    EXPECT_EQ(0U, filtered.size());
  }
}

// Tests if observer works as expected.
TEST_F(ArcIntentHelperTest, TestObserver) {
  class FakeObserver : public ArcIntentHelperObserver {
   public:
    FakeObserver() = default;
    void OnIntentFiltersUpdated() override { updated_ = true; }
    bool IsUpdated() { return updated_; }
    void Reset() { updated_ = false; }

   private:
    bool updated_ = false;
  };

  // Observer should be called when intent filter is updated.
  auto observer = std::make_unique<FakeObserver>();
  instance_->AddObserver(observer.get());
  EXPECT_FALSE(observer->IsUpdated());
  instance_->OnIntentFiltersUpdated(std::vector<IntentFilter>());
  EXPECT_TRUE(observer->IsUpdated());

  // Observer should not be called after it's removed.
  observer->Reset();
  instance_->RemoveObserver(observer.get());
  instance_->OnIntentFiltersUpdated(std::vector<IntentFilter>());
  EXPECT_FALSE(observer->IsUpdated());
}

// Tests that ShouldChromeHandleUrl returns true by default.
TEST_F(ArcIntentHelperTest, TestDefault) {
  EXPECT_TRUE(instance_->ShouldChromeHandleUrl(GURL("http://www.google.com")));
  EXPECT_TRUE(instance_->ShouldChromeHandleUrl(GURL("https://www.google.com")));
  EXPECT_TRUE(instance_->ShouldChromeHandleUrl(GURL("file:///etc/password")));
  EXPECT_TRUE(instance_->ShouldChromeHandleUrl(GURL("chrome://help")));
  EXPECT_TRUE(instance_->ShouldChromeHandleUrl(GURL("about://chrome")));
}

// Tests that ShouldChromeHandleUrl returns false when there's a match.
TEST_F(ArcIntentHelperTest, TestSingleFilter) {
  std::vector<IntentFilter> array;
  array.emplace_back(GetIntentFilter("www.google.com"));
  instance_->OnIntentFiltersUpdated(std::move(array));

  EXPECT_FALSE(instance_->ShouldChromeHandleUrl(GURL("http://www.google.com")));
  EXPECT_FALSE(
      instance_->ShouldChromeHandleUrl(GURL("https://www.google.com")));

  EXPECT_TRUE(
      instance_->ShouldChromeHandleUrl(GURL("https://www.google.co.uk")));
}

// Tests the same with multiple filters.
TEST_F(ArcIntentHelperTest, TestMultipleFilters) {
  std::vector<IntentFilter> array;
  array.emplace_back(GetIntentFilter("www.google.com"));
  array.emplace_back(GetIntentFilter("www.google.co.uk"));
  array.emplace_back(GetIntentFilter("dev.chromium.org"));
  instance_->OnIntentFiltersUpdated(std::move(array));

  EXPECT_FALSE(instance_->ShouldChromeHandleUrl(GURL("http://www.google.com")));
  EXPECT_FALSE(
      instance_->ShouldChromeHandleUrl(GURL("https://www.google.com")));
  EXPECT_FALSE(
      instance_->ShouldChromeHandleUrl(GURL("http://www.google.co.uk")));
  EXPECT_FALSE(
      instance_->ShouldChromeHandleUrl(GURL("https://www.google.co.uk")));
  EXPECT_FALSE(
      instance_->ShouldChromeHandleUrl(GURL("http://dev.chromium.org")));
  EXPECT_FALSE(
      instance_->ShouldChromeHandleUrl(GURL("https://dev.chromium.org")));

  EXPECT_TRUE(instance_->ShouldChromeHandleUrl(GURL("http://www.android.com")));
}

// Tests that ShouldChromeHandleUrl returns true for non http(s) URLs.
TEST_F(ArcIntentHelperTest, TestNonHttp) {
  std::vector<IntentFilter> array;
  array.emplace_back(GetIntentFilter("www.google.com"));
  instance_->OnIntentFiltersUpdated(std::move(array));

  EXPECT_TRUE(
      instance_->ShouldChromeHandleUrl(GURL("chrome://www.google.com")));
  EXPECT_TRUE(
      instance_->ShouldChromeHandleUrl(GURL("custom://www.google.com")));
}

// Tests that ShouldChromeHandleUrl discards the previous filters when
// UpdateIntentFilters is called with new ones.
TEST_F(ArcIntentHelperTest, TestMultipleUpdate) {
  std::vector<IntentFilter> array;
  array.emplace_back(GetIntentFilter("www.google.com"));
  array.emplace_back(GetIntentFilter("dev.chromium.org"));
  instance_->OnIntentFiltersUpdated(std::move(array));

  std::vector<IntentFilter> array2;
  array2.emplace_back(GetIntentFilter("www.google.co.uk"));
  array2.emplace_back(GetIntentFilter("dev.chromium.org"));
  array2.emplace_back(GetIntentFilter("www.android.com"));
  instance_->OnIntentFiltersUpdated(std::move(array2));

  EXPECT_TRUE(instance_->ShouldChromeHandleUrl(GURL("http://www.google.com")));
  EXPECT_TRUE(instance_->ShouldChromeHandleUrl(GURL("https://www.google.com")));
  EXPECT_FALSE(
      instance_->ShouldChromeHandleUrl(GURL("http://www.google.co.uk")));
  EXPECT_FALSE(
      instance_->ShouldChromeHandleUrl(GURL("https://www.google.co.uk")));
  EXPECT_FALSE(
      instance_->ShouldChromeHandleUrl(GURL("http://dev.chromium.org")));
  EXPECT_FALSE(
      instance_->ShouldChromeHandleUrl(GURL("https://dev.chromium.org")));
  EXPECT_FALSE(
      instance_->ShouldChromeHandleUrl(GURL("http://www.android.com")));
  EXPECT_FALSE(
      instance_->ShouldChromeHandleUrl(GURL("https://www.android.com")));
}

// Tests that OnOpenUrl opens the URL in Chrome browser.
TEST_F(ArcIntentHelperTest, TestOnOpenUrl) {
  instance_->OnOpenUrl("https://google.com");
  EXPECT_EQ(GURL("https://google.com"),
            test_shell_delegate_->GetLastOpenedUrlFromArc());
}

}  // namespace arc
