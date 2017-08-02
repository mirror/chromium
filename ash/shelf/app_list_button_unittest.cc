// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/app_list_button.h"

#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_view.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/command_line.h"
#include "base/i18n/rtl.h"
#include "base/test/scoped_command_line.h"
#include "chromeos/chromeos_switches.h"
#include "ui/app_list/presenter/app_list.h"
#include "ui/app_list/presenter/test/test_app_list_presenter.h"
#include "ui/events/event_constants.h"

namespace ash {

ui::GestureEvent CreateGestureEvent(ui::GestureEventDetails details) {
  return ui::GestureEvent(0, 0, ui::EF_NONE, base::TimeTicks(), details);
}

class AppListButtonTest : public AshTestBase {
 public:
  AppListButtonTest() {}
  ~AppListButtonTest() override {}

  void SetUp() override {
    command_line_ = base::MakeUnique<base::test::ScopedCommandLine>();
    SetupCommandLine(command_line_->GetProcessCommandLine());
    AshTestBase::SetUp();
    app_list_button_ =
        GetPrimaryShelf()->GetShelfViewForTesting()->GetAppListButton();
  }

  virtual void SetupCommandLine(base::CommandLine* command_line) {}

  void SendGestureEvent(ui::GestureEvent* event) {
    app_list_button_->OnGestureEvent(event);
  }

  AppListButton* GetAppListButton() { return app_list_button_; }

 private:
  AppListButton* app_list_button_;

  std::unique_ptr<base::test::ScopedCommandLine> command_line_;

  DISALLOW_COPY_AND_ASSIGN(AppListButtonTest);
};

TEST_F(AppListButtonTest, LongPressGestureWithoutVoiceInteractionFlag) {
  app_list::test::TestAppListPresenter test_app_list_presenter;
  Shell::Get()->app_list()->SetAppListPresenter(
      test_app_list_presenter.CreateInterfacePtrAndBind());
  ui::GestureEvent long_press =
      CreateGestureEvent(ui::GestureEventDetails(ui::ET_GESTURE_LONG_PRESS));
  SendGestureEvent(&long_press);
  RunAllPendingInMessageLoop();
  EXPECT_EQ(0u, test_app_list_presenter.voice_session_count());
}

class VoiceInteractionAppListButtonTest : public AppListButtonTest {
 public:
  VoiceInteractionAppListButtonTest() {}

  void SetupCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(chromeos::switches::kEnableVoiceInteraction);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(VoiceInteractionAppListButtonTest);
};

TEST_F(VoiceInteractionAppListButtonTest,
       LongPressGestureWithVoiceInteractionFlag) {
  app_list::test::TestAppListPresenter test_app_list_presenter;
  Shell::Get()->app_list()->SetAppListPresenter(
      test_app_list_presenter.CreateInterfacePtrAndBind());

  EXPECT_TRUE(base::CommandLine::ForCurrentProcess()->HasSwitch(
      chromeos::switches::kEnableVoiceInteraction));

  ui::GestureEvent long_press =
      CreateGestureEvent(ui::GestureEventDetails(ui::ET_GESTURE_LONG_PRESS));
  SendGestureEvent(&long_press);
  RunAllPendingInMessageLoop();
  EXPECT_EQ(1u, test_app_list_presenter.voice_session_count());
}

namespace {

class BackButtonAppListButtonTest : public AppListButtonTest,
                                    public testing::WithParamInterface<bool> {
 public:
  BackButtonAppListButtonTest() : is_rtl_(GetParam()) {}
  ~BackButtonAppListButtonTest() override {}

  void SetUp() override {
    original_locale_ = base::i18n::GetConfiguredLocale();
    if (is_rtl_)
      base::i18n::SetICUDefaultLocale("he");
    AppListButtonTest::SetUp();
    ASSERT_EQ(is_rtl_, base::i18n::IsRTL());
  }

  void TearDown() override {
    if (is_rtl_)
      base::i18n::SetICUDefaultLocale(original_locale_);
    AppListButtonTest::TearDown();
  }

 private:
  bool is_rtl_ = false;
  std::string original_locale_;

  DISALLOW_COPY_AND_ASSIGN(BackButtonAppListButtonTest);
};

INSTANTIATE_TEST_CASE_P(
    /* prefix intentionally left blank due to only one parameterization */,
    BackButtonAppListButtonTest,
    testing::Bool());

}  // namespace

// Verify the locations of the back button and app list button.
TEST_P(BackButtonAppListButtonTest, BackButtonAppListButtonLocation) {
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);

  bool called_by_draw_function = false;
  gfx::Point back_button_centre =
      GetAppListButton()->GetBackButtonCenterPoint(called_by_draw_function);
  gfx::Point app_list_button_centre =
      GetAppListButton()->GetAppListButtonCenterPoint(called_by_draw_function);

  // Verify that in rtl, the app list button centre has a smaller x value than
  // the back button centre and in ltr, the app list button centre has a larger
  // x value than the back button centre.
  if (base::i18n::IsRTL())
    EXPECT_LT(app_list_button_centre.x(), back_button_centre.x());
  else
    EXPECT_GT(app_list_button_centre.x(), back_button_centre.x());

  // Verify that when called by the draw function, the app list button centre x
  // value is always larger than the back button centre x value, regardless of
  // ltr or rtl.
  called_by_draw_function = true;
  back_button_centre =
      GetAppListButton()->GetBackButtonCenterPoint(called_by_draw_function);
  app_list_button_centre =
      GetAppListButton()->GetAppListButtonCenterPoint(called_by_draw_function);
  EXPECT_GT(app_list_button_centre.x(), back_button_centre.x());

  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(false);
}

}  // namespace ash
