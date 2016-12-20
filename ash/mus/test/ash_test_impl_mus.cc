// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/mus/test/ash_test_impl_mus.h"

#include "ash/common/session/session_controller.h"
#include "ash/common/test/ash_test.h"
#include "ash/common/wm_shell.h"
#include "ash/mus/bridge/wm_window_mus.h"
#include "ash/public/cpp/session_types.h"
#include "ash/public/interfaces/session_controller.mojom.h"
#include "base/memory/ptr_util.h"
#include "services/ui/public/cpp/property_type_converters.h"
#include "services/ui/public/interfaces/window_manager.mojom.h"
#include "ui/wm/core/window_util.h"

namespace ash {
namespace mus {
namespace {

// WmTestBase is abstract as TestBody() is pure virtual (the various TEST
// macros have the implementation). In order to create WmTestBase we have to
// subclass with an empty implementation of TestBody(). That's ok as the class
// isn't used as a normal test here.
class WmTestBaseImpl : public WmTestBase {
 public:
  WmTestBaseImpl() {}
  ~WmTestBaseImpl() override {}

  // WmTestBase:
  void TestBody() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(WmTestBaseImpl);
};

}  // namespace

AshTestImplMus::AshTestImplMus()
    : wm_test_base_(base::MakeUnique<WmTestBaseImpl>()) {}

AshTestImplMus::~AshTestImplMus() {}

void AshTestImplMus::SetUp() {
  wm_test_base_->SetUp();

  // Most tests assume the user is logged in (and hence the shelf is created).
  SimulateUserLogin();
}

void AshTestImplMus::TearDown() {
  wm_test_base_->TearDown();
}

bool AshTestImplMus::SupportsMultipleDisplays() const {
  return wm_test_base_->SupportsMultipleDisplays();
}

void AshTestImplMus::UpdateDisplay(const std::string& display_spec) {
  wm_test_base_->UpdateDisplay(display_spec);
}

std::unique_ptr<WindowOwner> AshTestImplMus::CreateTestWindow(
    const gfx::Rect& bounds_in_screen,
    ui::wm::WindowType type,
    int shell_window_id) {
  WmWindowMus* window =
      WmWindowMus::Get(wm_test_base_->CreateTestWindow(bounds_in_screen, type));
  window->SetShellWindowId(shell_window_id);
  return base::MakeUnique<WindowOwner>(window);
}

std::unique_ptr<WindowOwner> AshTestImplMus::CreateToplevelTestWindow(
    const gfx::Rect& bounds_in_screen,
    int shell_window_id) {
  // For mus CreateTestWindow() creates top level windows (assuming
  // WINDOW_TYPE_NORMAL).
  return CreateTestWindow(bounds_in_screen, ui::wm::WINDOW_TYPE_NORMAL,
                          shell_window_id);
}

display::Display AshTestImplMus::GetSecondaryDisplay() {
  return wm_test_base_->GetSecondaryDisplay();
}

bool AshTestImplMus::SetSecondaryDisplayPlacement(
    display::DisplayPlacement::Position position,
    int offset) {
  NOTIMPLEMENTED();
  return false;
}

void AshTestImplMus::ConfigureWidgetInitParamsForDisplay(
    WmWindow* window,
    views::Widget::InitParams* init_params) {
  init_params->context = WmWindowMus::GetAuraWindow(window);
  init_params
      ->mus_properties[ui::mojom::WindowManager::kDisplayId_InitProperty] =
      mojo::ConvertTo<std::vector<uint8_t>>(
          window->GetDisplayNearestWindow().id());
}

void AshTestImplMus::AddTransientChild(WmWindow* parent, WmWindow* window) {
  // TODO(sky): remove this as both classes can share same implementation now.
  ::wm::AddTransientChild(WmWindowAura::GetAuraWindow(parent),
                          WmWindowAura::GetAuraWindow(window));
}

void AshTestImplMus::SimulateUserLogin() {
  SessionController* session_controller = WmShell::Get()->session_controller();

  // Simulate the first user logging in.
  mojom::UserSessionPtr session = mojom::UserSession::New();
  session->session_id = 1;
  session->type = user_manager::USER_TYPE_REGULAR;
  const std::string email("ash.user@example.com");
  session->serialized_account_id = AccountId::FromUserEmail(email).Serialize();
  session->display_name = "Ash User";
  session->display_email = email;
  session_controller->UpdateUserSession(std::move(session));

  // Simulate the user session becoming active.
  mojom::SessionInfoPtr info = mojom::SessionInfo::New();
  info->max_users = 10;
  info->can_lock_screen = true;
  info->should_lock_screen_automatically = false;
  info->add_user_session_policy = AddUserSessionPolicy::ALLOWED;
  info->state = session_manager::SessionState::ACTIVE;
  session_controller->SetSessionInfo(std::move(info));
}

}  // namespace mus

// static
std::unique_ptr<AshTestImpl> AshTestImpl::Create() {
  return base::MakeUnique<mus::AshTestImplMus>();
}

}  // namespace ash
