// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/toolbar/app_toolbar_button.h"

#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_param_associator.h"
#include "base/metrics/field_trial_params.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/ui/cocoa/animated_icon.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "chrome/common/chrome_features.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

class TestAnimatedIcon : public AnimatedIcon {
 public:
  TestAnimatedIcon();
  void Animate() override;
  bool IsAnimating() const override;

  void ResetAnimatingFlag();

  bool is_animating() const { return is_animating_; }

 private:
  bool is_animating_;
  DISALLOW_COPY_AND_ASSIGN(TestAnimatedIcon);
};

TestAnimatedIcon::TestAnimatedIcon()
    : AnimatedIcon(kBrowserToolsAnimatedIcon, nil), is_animating_(false) {}

void TestAnimatedIcon::Animate() {
  is_animating_ = true;
}

bool TestAnimatedIcon::IsAnimating() const {
  return is_animating_;
}

void TestAnimatedIcon::ResetAnimatingFlag() {
  is_animating_ = false;
}

class AppToolbarButtonTest : public CocoaTest {
 protected:
  AppToolbarButtonTest()
      : field_trial_list_(new base::FieldTrialList(nullptr)) {}

  void EnableFeatureList(bool with_delay) {
    field_trial_list_.reset();
    field_trial_list_.reset(new base::FieldTrialList(nullptr));

    base::FieldTrialParamAssociator::GetInstance()->ClearAllParamsForTesting();
    const char kTrialName[] = "TrialName";
    const char kGroupName[] = "GroupName";

    std::map<std::string, std::string> params;
    params["HasDelay"] = with_delay ? "true" : "false";
    ASSERT_TRUE(
        base::AssociateFieldTrialParams(kTrialName, kGroupName, params));
    base::FieldTrial* field_trial =
        base::FieldTrialList::CreateFieldTrial(kTrialName, kGroupName);

    std::unique_ptr<base::FeatureList> feature_list(
        base::MakeUnique<base::FeatureList>());
    feature_list->RegisterFieldTrialOverride(
        features::kAnimatedAppMenuIcon.name,
        base::FeatureList::OVERRIDE_ENABLE_FEATURE, field_trial);
    scoped_feature_list_.InitWithFeatureList(std::move(feature_list));
  }

  void SetSeverityLevel(AppMenuIconController::Severity level) {
    [button_ setSeverity:level
                iconType:AppMenuIconController::IconType::NONE
           shouldAnimate:YES];
  }

  base::test::ScopedFeatureList scoped_feature_list_;

  std::unique_ptr<base::FieldTrialList> field_trial_list_;

  base::scoped_nsobject<AppToolbarButton> button_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AppToolbarButtonTest);
};

TEST_F(AppToolbarButtonTest, OldAppToolbarIcon) {
  scoped_feature_list_.InitAndDisableFeature(features::kAnimatedAppMenuIcon);
  button_.reset([[AppToolbarButton alloc] initWithFrame:NSZeroRect]);
  EXPECT_FALSE([button_ animatedIcon]);
}

TEST_F(AppToolbarButtonTest, NewAppToolbarIcon_NoDelay) {
  EnableFeatureList(false);
  button_.reset([[AppToolbarButton alloc] initWithFrame:NSZeroRect]);
  EXPECT_TRUE([button_ animatedIcon]);

  TestAnimatedIcon* test_icon = new TestAnimatedIcon();
  [button_ setAnimatedIcon:test_icon];
  [button_ setDisableTimerForTest:YES];

  // Should not animate.
  SetSeverityLevel(AppMenuIconController::Severity::NONE);
  EXPECT_FALSE([button_ animationDelayTimer]);
  EXPECT_FALSE(test_icon->is_animating());

  test_icon->ResetAnimatingFlag();

  SetSeverityLevel(AppMenuIconController::Severity::LOW);
  EXPECT_FALSE([button_ animationDelayTimer]);
  EXPECT_TRUE(test_icon->is_animating());

  test_icon->ResetAnimatingFlag();

  SetSeverityLevel(AppMenuIconController::Severity::MEDIUM);
  EXPECT_FALSE([button_ animationDelayTimer]);
  EXPECT_TRUE(test_icon->is_animating());

  test_icon->ResetAnimatingFlag();

  SetSeverityLevel(AppMenuIconController::Severity::HIGH);
  EXPECT_FALSE([button_ animationDelayTimer]);
  EXPECT_TRUE(test_icon->is_animating());
}

TEST_F(AppToolbarButtonTest, NewAppToolbarIcon_HasDelay) {
  EnableFeatureList(true);
  button_.reset([[AppToolbarButton alloc] initWithFrame:NSZeroRect]);
  EXPECT_TRUE([button_ animatedIcon]);

  TestAnimatedIcon* test_icon = new TestAnimatedIcon();
  [button_ setAnimatedIcon:test_icon];
  [button_ setDisableTimerForTest:YES];

  // Should not animate.
  SetSeverityLevel(AppMenuIconController::Severity::NONE);
  EXPECT_FALSE([button_ animationDelayTimer]);
  EXPECT_FALSE(test_icon->is_animating());

  test_icon->ResetAnimatingFlag();

  SetSeverityLevel(AppMenuIconController::Severity::LOW);
  EXPECT_TRUE([button_ animationDelayTimer]);
  EXPECT_FALSE(test_icon->is_animating());

  test_icon->ResetAnimatingFlag();

  SetSeverityLevel(AppMenuIconController::Severity::MEDIUM);
  EXPECT_TRUE([button_ animationDelayTimer]);
  EXPECT_FALSE(test_icon->is_animating());

  test_icon->ResetAnimatingFlag();

  SetSeverityLevel(AppMenuIconController::Severity::HIGH);
  EXPECT_TRUE([button_ animationDelayTimer]);
  EXPECT_FALSE(test_icon->is_animating());

  [button_ animateIfPossibleWithDelay:NO];
  EXPECT_FALSE([button_ animationDelayTimer]);
  EXPECT_TRUE(test_icon->is_animating());

  [button_ animateIfPossibleWithDelay:YES];
  EXPECT_FALSE([button_ animationDelayTimer]);
  EXPECT_TRUE(test_icon->is_animating());
}
