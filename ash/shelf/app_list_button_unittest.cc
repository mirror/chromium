// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/app_list_button.h"

#include "ash/public/interfaces/session_controller.mojom.h"
#include "ash/session/session_controller.h"
#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_view.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "base/command_line.h"
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
  ~AppListButtonTest() override {
    controller_->RemoveObserver(app_list_button_);
  }

  void SetUp() override {
    command_line_ = base::MakeUnique<base::test::ScopedCommandLine>();
    SetupCommandLine(command_line_->GetProcessCommandLine());
    AshTestBase::SetUp();
    app_list_button_ =
        GetPrimaryShelf()->GetShelfViewForTesting()->GetAppListButton();

    controller_ = base::MakeUnique<SessionController>();
    controller_->AddObserver(app_list_button_);
  }

  void UpdateSession(uint32_t session_id, const std::string& email) {
    mojom::UserSessionPtr session = mojom::UserSession::New();
    session->session_id = session_id;
    session->user_info = mojom::UserInfo::New();
    session->user_info->type = user_manager::USER_TYPE_REGULAR;
    session->user_info->account_id = AccountId::FromUserEmail(email);
    session->user_info->display_name = email;
    session->user_info->display_email = email;

    controller_->UpdateUserSession(std::move(session));
  }

  virtual void SetupCommandLine(base::CommandLine* command_line) {}

  void SendGestureEvent(ui::GestureEvent* event) {
    app_list_button_->OnGestureEvent(event);
  }

  void ChangeActiveUserSession() {
    UpdateSession(1u, "user1@test.com");
    UpdateSession(2u, "user2@test.com");

    std::vector<uint32_t> order = {1u, 2u};
    controller_->SetUserSessionOrder(order);
  }

 private:
  AppListButton* app_list_button_;
  std::unique_ptr<base::test::ScopedCommandLine> command_line_;
  std::unique_ptr<SessionController> controller_;

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
  ChangeActiveUserSession();

  EXPECT_TRUE(base::CommandLine::ForCurrentProcess()->HasSwitch(
      chromeos::switches::kEnableVoiceInteraction));

  ui::GestureEvent long_press =
      CreateGestureEvent(ui::GestureEventDetails(ui::ET_GESTURE_LONG_PRESS));
  SendGestureEvent(&long_press);
  RunAllPendingInMessageLoop();
  EXPECT_EQ(1u, test_app_list_presenter.voice_session_count());
}

}  // namespace ash
