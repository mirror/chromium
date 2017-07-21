// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/ui/login_auth_user_view.h"

#include "ash/login/lock_screen_controller.h"
#include "ash/login/ui/lock_screen.h"
#include "ash/login/ui/login_constants.h"
#include "ash/login/ui/login_display_style.h"
#include "ash/login/ui/login_password_view.h"
#include "ash/login/ui/login_pin_view.h"
#include "ash/login/ui/login_user_view.h"
#include "ash/login/ui/pin_keyboard_animation.h"
#include "ash/shell.h"
#include "base/strings/utf_string_conversions.h"
#include "components/user_manager/user.h"
#include "ui/compositor/callback_layer_animation_observer.h"
#include "ui/compositor/layer_animation_sequence.h"
#include "ui/compositor/layer_animator.h"
#include "ui/views/animation/bounds_animator.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"

namespace ash {
namespace {

const char* kLoginAuthUserViewClassName = "LoginAuthUserView";

// Any non-zero value used for separator width. Makes debugging easier.
constexpr int kNonEmptyWidthDp = 30;

// Distance between the username label and the password textfield.
const int kDistanceBetweenUsernameAndPasswordDp = 28;

// Distance between the password textfield and the the pin keyboard.
const int kDistanceBetweenPasswordFieldAndPinKeyboard = 20;

// Distance from the end of pin keyboard to the bottom of the big user view.
const int kDistanceFromPinKeyboardToBigUserViewBottom = 48;

views::View* CreateViewOfHeight(int height) {
  auto* view = new views::View();
  view->SetPreferredSize(gfx::Size(kNonEmptyWidthDp, height));
  return view;
}

// Returns an observer that will hide |view| when it fires. The observer will
// delete itself after firing. Make sure to call |observer->SetReady()| after
// attaching it.
ui::CallbackLayerAnimationObserver* BuildObserverToHideView(views::View* view) {
  return new ui::CallbackLayerAnimationObserver(base::Bind(
      [](views::View* view,
         const ui::CallbackLayerAnimationObserver& observer) {
        // Don't hide the view if the animation is aborted, as |view| may no
        // longer be valid.
        if (observer.aborted_count())
          return true;

        view->SetVisible(false);
        return true;
      },
      view));
}

// Ensures that |view| has a non-opaque layer.
void EnsureTransparentLayer(views::View* view) {
  if (!view->layer()) {
    view->SetPaintToLayer();
    view->layer()->SetFillsBoundsOpaquely(false);
  }
}

}  // namespace

struct LoginAuthUserView::AnimationState {
  gfx::Rect non_pin_screen_bounds_start;
  int pin_y_start = 0;
  bool had_pin = false;
  bool had_password = false;

  explicit AnimationState(LoginAuthUserView* view) {
    non_pin_screen_bounds_start = view->non_pin_root_->GetBoundsInScreen();
    pin_y_start = view->pin_view_->GetBoundsInScreen().y();

    had_pin = (view->auth_methods() & LoginAuthUserView::AUTH_PIN) != 0;
    had_password =
        (view->auth_methods() & LoginAuthUserView::AUTH_PASSWORD) != 0;
  }
};

LoginAuthUserView::TestApi::TestApi(LoginAuthUserView* view) : view_(view) {}

LoginAuthUserView::TestApi::~TestApi() = default;

LoginUserView* LoginAuthUserView::TestApi::user_view() const {
  return view_->user_view_;
}

LoginPasswordView* LoginAuthUserView::TestApi::password_view() const {
  return view_->password_view_;
}

LoginAuthUserView::LoginAuthUserView(const mojom::UserInfoPtr& user,
                                     const OnAuthCallback& on_auth,
                                     const LoginUserView::OnTap& on_tap)
    : on_auth_(on_auth) {
  // Build child views.
  user_view_ = new LoginUserView(LoginDisplayStyle::kLarge,
                                 true /*show_dropdown*/, on_tap);
  password_view_ = new LoginPasswordView(
      base::Bind(&LoginAuthUserView::OnAuthSubmit, base::Unretained(this),
                 false /*is_pin*/));
  pin_view_ =
      new LoginPinView(base::BindRepeating(&LoginPasswordView::AppendNumber,
                                           base::Unretained(password_view_)),
                       base::BindRepeating(&LoginPasswordView::Backspace,
                                           base::Unretained(password_view_)));

  // Build layout.
  SetLayoutManager(new views::BoxLayout(views::BoxLayout::kVertical));

  non_pin_root_ = new views::View();
  non_pin_root_->SetLayoutManager(
      new views::BoxLayout(views::BoxLayout::kVertical, gfx::Insets(),
                           kDistanceBetweenUsernameAndPasswordDp));

  AddChildView(non_pin_root_);

  // Note: |user_view_| will be sized to it's minimum size (not its preferred
  // size) because of the vertical box layout manager. This class expresses the
  // minimum preferred size again so everything works out as desired (ie, we can
  // control how far away the password auth is from the user label).
  non_pin_root_->AddChildView(user_view_);

  AddChildView(CreateViewOfHeight(kDistanceBetweenUsernameAndPasswordDp));

  {
    // We need to center LoginPasswordAuth.
    //
    // Also, BoxLayout::kVertical will ignore preferred width, which messes up
    // separator rendering.
    auto* row = new views::View();
    non_pin_root_->AddChildView(row);

    auto* layout = new views::BoxLayout(views::BoxLayout::kHorizontal);
    layout->set_main_axis_alignment(
        views::BoxLayout::MAIN_AXIS_ALIGNMENT_CENTER);
    row->SetLayoutManager(layout);

    // Proxy |password_view_| inside a container so that when we hide
    // |password_view_| is still consumes the same amount of space in layout.
    auto* password_view_container = new views::View();
    password_view_container->SetLayoutManager(new views::FillLayout());
    password_view_container->SetPreferredSize(
        password_view_->GetPreferredSize());
    password_view_container->AddChildView(password_view_);
    row->AddChildView(password_view_container);
  }

  AddChildView(CreateViewOfHeight(kDistanceBetweenPasswordFieldAndPinKeyboard));

  {
    // We need to center LoginPinAuth.
    auto* row = new views::View();
    AddChildView(row);

    auto* layout = new views::BoxLayout(views::BoxLayout::kHorizontal);
    layout->set_main_axis_alignment(
        views::BoxLayout::MAIN_AXIS_ALIGNMENT_CENTER);
    row->SetLayoutManager(layout);

    row->AddChildView(pin_view_);
  }

  AddChildView(CreateViewOfHeight(kDistanceFromPinKeyboardToBigUserViewBottom));

  SetAuthMethods(auth_methods_);
  user_view_->UpdateForUser(user, false /*animate*/);

  // Create animator for non-PIN content.
  animator_ = base::MakeUnique<views::BoundsAnimator>(non_pin_root_->parent());
  animator_->SetAnimationDuration(kChangeUserAnimationDurationMs);
  animator_->set_tween_type(gfx::Tween::FAST_OUT_SLOW_IN);
}

LoginAuthUserView::~LoginAuthUserView() = default;

void LoginAuthUserView::SetAuthMethods(uint32_t auth_methods) {
  // TODO(jdufault): Implement additional auth methods.
  auth_methods_ = static_cast<AuthMethods>(auth_methods);
  password_view_->SetVisible(auth_methods_ & AUTH_PASSWORD);
  pin_view_->SetVisible(auth_methods_ & AUTH_PIN);
  PreferredSizeChanged();
}

void LoginAuthUserView::CaptureStateForAnimationPreLayout() {
  DCHECK(!cached_animation_state_);
  cached_animation_state_ = base::MakeUnique<AnimationState>(this);
}

void LoginAuthUserView::ApplyAnimationPostLayout() {
  DCHECK(cached_animation_state_);

  // Cancel any running animations.
  if (pin_view_->layer())
    pin_view_->layer()->GetAnimator()->AbortAllAnimations();
  if (password_view_->layer())
    password_view_->layer()->GetAnimator()->AbortAllAnimations();
  animator_->Cancel();

  bool has_password = (auth_methods() & AUTH_PASSWORD) != 0;
  bool has_pin = (auth_methods() & AUTH_PIN) != 0;

  ////////
  // Animate the user info (ie, icon, name) up or down the screen.

  gfx::Rect bounds_end = non_pin_root_->bounds();

  // Move the user info where is used to be by offseting the relative position
  // by the deltas in the absolutely positioned coordinates.
  gfx::Rect bounds_start = bounds_end;
  gfx::Rect non_pin_screen_bounds_end = non_pin_root_->GetBoundsInScreen();
  bounds_start.set_y(cached_animation_state_->non_pin_screen_bounds_start.y() -
                     non_pin_screen_bounds_end.y());
  bounds_start.set_height(
      cached_animation_state_->non_pin_screen_bounds_start.height());

  non_pin_root_->SetBoundsRect(bounds_start);
  animator_->AnimateViewTo(non_pin_root_, bounds_end);

  ////////
  // Fade the password view if it is being hidden or shown.

  if (cached_animation_state_->had_password != has_password) {
    EnsureTransparentLayer(password_view_);

    // Ensure the password view is visible so we can animate it.
    if (!has_password)
      password_view_->SetVisible(true);

    float opacity_start = 0, opacity_end = 1;
    if (!has_password)
      std::swap(opacity_start, opacity_end);

    password_view_->layer()->SetOpacity(opacity_start);
    auto transition = ui::LayerAnimationElement::CreateOpacityElement(
        opacity_end,
        base::TimeDelta::FromMilliseconds(kChangeUserAnimationDurationMs));
    transition->set_tween_type(gfx::Tween::Type::FAST_OUT_SLOW_IN);
    auto* sequence = new ui::LayerAnimationSequence(std::move(transition));

    password_view_->layer()->GetAnimator()->ScheduleAnimation(sequence);

    // Hide the password after animation if needed.
    if (!has_password) {
      auto* observer = BuildObserverToHideView(password_view_);
      sequence->AddObserver(observer);
      observer->SetActive();
    }
  }

  ////////
  // Grow/shrink the PIN keyboard if it is being hidden or shown.

  if (cached_animation_state_->had_pin != has_pin) {
    if (!has_pin) {
      // Non-PIN content will be painted outside of its bounds as it animates
      // to the correct position. This requires a layer to display properly.
      EnsureTransparentLayer(non_pin_root_);

      int pin_y_end = pin_view_->GetBoundsInScreen().y();
      gfx::Rect pin_bounds = pin_view_->bounds();
      pin_bounds.set_y(cached_animation_state_->pin_y_start - pin_y_end);

      // Since PIN is disabled, the previous Layout() hid the PIN keyboard.
      // We need to redisplay it where it used to be.
      pin_view_->SetVisible(true);
      pin_view_->SetBoundsRect(pin_bounds);
    }

    EnsureTransparentLayer(pin_view_);
    auto transition = base::MakeUnique<PinKeyboardAnimation>(
        has_pin /*grow*/, pin_view_->height(),
        base::TimeDelta::FromMilliseconds(kChangeUserAnimationDurationMs),
        gfx::Tween::FAST_OUT_SLOW_IN);
    auto* sequence = new ui::LayerAnimationSequence(std::move(transition));
    pin_view_->layer()->GetAnimator()->ScheduleAnimation(sequence);

    // Hide the PIN keyboard after animation if needed.
    if (!has_pin) {
      auto* observer = BuildObserverToHideView(pin_view_);
      sequence->AddObserver(observer);
      observer->SetActive();
    }
  }

  cached_animation_state_.reset();
}

void LoginAuthUserView::UpdateForUser(const mojom::UserInfoPtr& user) {
  user_view_->UpdateForUser(user, true /*animate*/);
}

const mojom::UserInfoPtr& LoginAuthUserView::current_user() const {
  return user_view_->current_user();
}

const char* LoginAuthUserView::GetClassName() const {
  return kLoginAuthUserViewClassName;
}

gfx::Size LoginAuthUserView::CalculatePreferredSize() const {
  gfx::Size size = views::View::CalculatePreferredSize();
  // Make sure we are at least as big as the user view. If we do not do this the
  // view will be below minimum size when no auth methods are displayed.
  size.SetToMax(user_view_->GetPreferredSize());
  return size;
}

void LoginAuthUserView::OnAuthSubmit(bool is_pin,
                                     const base::string16& password) {
  Shell::Get()->lock_screen_controller()->AuthenticateUser(
      current_user()->account_id, UTF16ToUTF8(password), is_pin,
      base::BindOnce([](OnAuthCallback on_auth,
                        bool auth_success) { on_auth.Run(auth_success); },
                     on_auth_));
}

}  // namespace ash
