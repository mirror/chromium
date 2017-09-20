// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/ui/login_password_view.h"

#include "ash/login/ui/non_accessible_view.h"
#include "chrome/browser/image_decoder.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/tray/size_range_layout.h"
#include "ash/system/user/button_from_view.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/layer_animation_element.h"
#include "ui/compositor/layer_animation_sequence.h"
#include "ui/compositor/layer_animator.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/events/event_constants.h"
#include "ui/events/keycodes/dom/dom_code.h"
#include "ui/gfx/interpolated_transform.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/resources/grit/ui_resources.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/separator.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"

// TODO(jdufault): On two user view the password prompt is visible to
// accessibility using special navigation keys even though it is invisible. We
// probably need to customize the text box quite a bit, we may want to do
// something similar to SearchBoxView.

namespace ash {
namespace {

// Width/height of the submit button.
constexpr int kSubmitButtonWidthDp = 20;
constexpr int kSubmitButtonHeightDp = 20;

// Width/height of the password inputfield.
constexpr int kPasswordInputWidthDp = 184;
constexpr int kPasswordInputHeightDp = 36;

// Total width of the password view.
constexpr int kPasswordTotalWidthDp = 204;

// Distance between the last password dot and the submit arrow/button.
constexpr int kDistanceBetweenPasswordAndSubmitDp = 0;

// Width and height of the easy unlock icon.
constexpr const int kEasyUnlockIconSizeDp = 16;

// Horizontal distance/margin between the easy unlock icon and the start of
// the password view.
constexpr const int kHorizontalDistanceBetweenEasyUnlockAndPasswordDp = 12;

// Non-empty height, useful for debugging/visualization.
constexpr const int kNonEmptyHeight = 1;

constexpr const char kLoginPasswordViewName[] = "LoginPasswordView";

class NonAccessibleSeparator : public views::Separator {
 public:
  NonAccessibleSeparator() = default;
  ~NonAccessibleSeparator() override = default;

  // views::Separator:
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override {
    views::Separator::GetAccessibleNodeData(node_data);
    node_data->AddState(ui::AX_STATE_INVISIBLE);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(NonAccessibleSeparator);
};

int GetEasyUnlockResource(LoginPasswordView::EasyUnlockState state) {
  switch (state) {
    case LoginPasswordView::EasyUnlockState::kNone:
      break;
    case LoginPasswordView::EasyUnlockState::kHardlocked:
      return IDR_EASY_UNLOCK_HARDLOCKED;
    case LoginPasswordView::EasyUnlockState::kHardlockedHover:
      return IDR_EASY_UNLOCK_HARDLOCKED_HOVER;
    case LoginPasswordView::EasyUnlockState::kHardlockedPressed:
      return IDR_EASY_UNLOCK_HARDLOCKED_PRESSED;
    case LoginPasswordView::EasyUnlockState::kLocked:
      return IDR_EASY_UNLOCK_LOCKED;
    case LoginPasswordView::EasyUnlockState::kLockedHover:
      return IDR_EASY_UNLOCK_LOCKED_HOVER;
    case LoginPasswordView::EasyUnlockState::kLockedPressed:
      return IDR_EASY_UNLOCK_LOCKED_PRESSED;
    case LoginPasswordView::EasyUnlockState::kLockedToBeActivated:
      return IDR_EASY_UNLOCK_LOCKED_TO_BE_ACTIVATED;
    case LoginPasswordView::EasyUnlockState::kLockedToBeActivatedHover:
      return IDR_EASY_UNLOCK_LOCKED_TO_BE_ACTIVATED_HOVER;
    case LoginPasswordView::EasyUnlockState::kLockedToBeActivatedPressed:
      return IDR_EASY_UNLOCK_LOCKED_TO_BE_ACTIVATED_PRESSED;
    case LoginPasswordView::EasyUnlockState::kLockedWithProximityHint:
      return IDR_EASY_UNLOCK_LOCKED_WITH_PROXIMITY_HINT;
    case LoginPasswordView::EasyUnlockState::kLockedWithProximityHintHover:
      return IDR_EASY_UNLOCK_LOCKED_WITH_PROXIMITY_HINT_HOVER;
    case LoginPasswordView::EasyUnlockState::kLockedWithProximityHintPressed:
      return IDR_EASY_UNLOCK_LOCKED_WITH_PROXIMITY_HINT_PRESSED;
    case LoginPasswordView::EasyUnlockState::kSpinner:
      return IDR_EASY_UNLOCK_SPINNER;
    case LoginPasswordView::EasyUnlockState::kUnlocked:
      return IDR_EASY_UNLOCK_UNLOCKED;
    case LoginPasswordView::EasyUnlockState::kUnlockedHover:
      return IDR_EASY_UNLOCK_UNLOCKED_HOVER;
    case LoginPasswordView::EasyUnlockState::kUnlockedPressed:
      return IDR_EASY_UNLOCK_UNLOCKED_PRESSED;
  };

  NOTREACHED();
  return IDR_EASY_UNLOCK_LOCKED;
}

bool ShouldSpin(LoginPasswordView::EasyUnlockState state) {
  return state == LoginPasswordView::EasyUnlockState::kSpinner;
}

std::unique_ptr<ui::InterpolatedTransform> BuildRotation(int icon_size) {
  // Set rotation pivot to center.
  gfx::Transform to_center;
  to_center.Translate(icon_size / 2.f, icon_size / 2.f);
  auto move_to_center =
      base::MakeUnique<ui::InterpolatedConstantTransform>(to_center);

  // Rotate.
  auto rotate = base::MakeUnique<ui::InterpolatedRotation>(0, 360);
  rotate->SetChild(std::move(move_to_center));

  // Undo rotation pivot.
  gfx::Transform from_center;
  from_center.Translate(-icon_size / 2.f, -icon_size / 2.f);
  auto move_from_center =
      base::MakeUnique<ui::InterpolatedConstantTransform>(from_center);
  move_from_center->SetChild(std::move(rotate));
  return move_from_center;
}

}  // namespace

LoginPasswordView::TestApi::TestApi(LoginPasswordView* view) : view_(view) {}

LoginPasswordView::TestApi::~TestApi() = default;

views::View* LoginPasswordView::TestApi::textfield() const {
  return view_->textfield_;
}

views::View* LoginPasswordView::TestApi::submit_button() const {
  return view_->submit_button_;
}

LoginPasswordView::LoginPasswordView(const OnPasswordSubmit& on_submit)
    : on_submit_(on_submit) {
  DCHECK(on_submit_);
  auto* root_layout = new views::BoxLayout(views::BoxLayout::kVertical);
  root_layout->set_main_axis_alignment(
      views::BoxLayout::MAIN_AXIS_ALIGNMENT_CENTER);
  SetLayoutManager(root_layout);

  auto* row = new NonAccessibleView();
  // row->SetBackground(views::CreateSolidBackground(SK_ColorRED));

  // DONOTSUBMIT
  LOG(ERROR) << kPasswordInputWidthDp << kPasswordInputHeightDp;

  constexpr const int kMarginAboveBelowPasswordIconsDp = 8;
  auto* layout =
      new views::BoxLayout(views::BoxLayout::kHorizontal,
                           gfx::Insets(kMarginAboveBelowPasswordIconsDp, 0));
  layout->set_main_axis_alignment(views::BoxLayout::MAIN_AXIS_ALIGNMENT_CENTER);
  layout->set_cross_axis_alignment(
      views::BoxLayout::CROSS_AXIS_ALIGNMENT_CENTER);
  row->SetLayoutManager(layout);
  // TODO: this is ignored
  // row->SetPreferredSize(gfx::Size(kPasswordInputWidthDp * 10,
  // kPasswordInputHeightDp));

  // auto* vertical_sizer = new NonAccessibleView();
  // vertical_sizer->SetLayoutManager(
  //     new views::BoxLayout(views::BoxLayout::kVertical));
  // vertical_sizer->SetPreferredSize(gfx::Size(1, kPasswordInputHeightDp *
  // 10)); vertical_sizer->AddChildView(row); AddChildView(vertical_sizer);
  AddChildView(row);

  // Add easy unlock icon.
  easy_unlock_icon_ = new views::ImageView();
  easy_unlock_icon_->SetPreferredSize(
      gfx::Size(kEasyUnlockIconSizeDp, kEasyUnlockIconSizeDp));
  row->AddChildView(easy_unlock_icon_);

  easy_unlock_right_margin_ = new NonAccessibleView();
  easy_unlock_right_margin_->SetPreferredSize(gfx::Size(
      kHorizontalDistanceBetweenEasyUnlockAndPasswordDp, kNonEmptyHeight));
  row->AddChildView(easy_unlock_right_margin_);

  // SetEasyUnlockIcon(EasyUnlockState::kNone); // DONOTSUBMIT
  SetEasyUnlockIcon(EasyUnlockState::kSpinner);

  // easy_unlock_icon_->SetVisible(false);
  // easy_unlock_right_margin_->SetVisible(false);

  // Password textfield. We control the textfield size by sizing the parent
  // view, as the textfield will expand to fill it.
  // auto* textfield_sizer = new NonAccessibleView();
  // textfield_sizer->SetLayoutManager(new SizeRangeLayout(gfx::Size(
  // kPasswordInputWidthDp - kEasyUnlockIconSizeDp, kPasswordInputHeightDp)));
  textfield_ = new views::Textfield();
  textfield_->SetTextInputType(ui::TEXT_INPUT_TYPE_PASSWORD);
  // TODO(jdufault): Real placeholder text.
  textfield_->set_placeholder_text(base::ASCIIToUTF16("Password (FIXME)"));
  textfield_->SetBorder(nullptr);
  textfield_->SetBackgroundColor(SK_ColorTRANSPARENT);

  // textfield_sizer->AddChildView(textfield_);
  // row->AddChildView(textfield_sizer);
  row->AddChildView(textfield_);
  layout->SetFlexForView(textfield_, 1);

  auto* spacer = new NonAccessibleView();
  spacer->SetPreferredSize(
      gfx::Size(kDistanceBetweenPasswordAndSubmitDp, kNonEmptyHeight));
  row->AddChildView(spacer);

  // Submit button.
  auto* submit_icon = new views::ImageView();
  submit_icon->SetPreferredSize(
      gfx::Size(kSubmitButtonWidthDp, kSubmitButtonHeightDp));
  // TODO: Change icon color when enabled/disabled.
  submit_icon->SetImage(
      gfx::CreateVectorIcon(kLockScreenArrowIcon, SK_ColorWHITE));
  submit_button_ = new tray::ButtonFromView(
      submit_icon, this, TrayPopupInkDropStyle::HOST_CENTERED);
  submit_button_->SetAccessibleName(l10n_util::GetStringUTF16(
      IDS_ASH_LOGIN_POD_SUBMIT_BUTTON_ACCESSIBLE_NAME));
  row->AddChildView(submit_button_);

  // Separator on bottom.
  auto* separator = new NonAccessibleSeparator();
  AddChildView(separator);

  // Make sure the textfield always starts with focus.
  textfield_->RequestFocus();
}

LoginPasswordView::~LoginPasswordView() = default;

void LoginPasswordView::SetEasyUnlockIcon(EasyUnlockState state) {
  easy_unlock_icon_->SetVisible(state != EasyUnlockState::kNone);
  easy_unlock_right_margin_->SetVisible(state != EasyUnlockState::kNone);
  if (state == EasyUnlockState::kNone)
    return;

  auto& rb = ResourceBundle::GetSharedInstance();

  // What about this API?
  //
  //    views::AnimatedImageView* v = new ...;
  //    v->SetImages(rb.GetAnimatedImageSkiaNamed(IDR_FOO));
  //
  //    rb.GetAnimatedImageSkiaNamed returns vector<gfx::ImageSkia*>

  base::StringPiece raw_apng =
      rb.GetRawDataResource(GetEasyUnlockResource(state));
  // might have to call mojo directly, image_decoder.mojom






  easy_unlock_icon_->SetImage(
      rb.GetImageSkiaNamed(GetEasyUnlockResource(state)));

  if (ShouldSpin(state)) {
    easy_unlock_icon_->SetPaintToLayer();
    easy_unlock_icon_->layer()->SetFillsBoundsOpaquely(false);

    // TODO/FIXME: it is actually an animated PNG with a bunch of keyframes.

    // Centered rotation.
    auto* sequence = new ui::LayerAnimationSequence(
        ui::LayerAnimationElement::CreateInterpolatedTransformElement(
            BuildRotation(kEasyUnlockIconSizeDp),
            base::TimeDelta::FromMilliseconds(2500)));
    sequence->set_is_cyclic(true);
    easy_unlock_icon_->layer()->GetAnimator()->StartAnimation(sequence);
  }
}

void LoginPasswordView::UpdateForUser(const mojom::UserInfoPtr& user) {
  textfield_->SetAccessibleName(l10n_util::GetStringFUTF16(
      IDS_ASH_LOGIN_POD_PASSWORD_FIELD_ACCESSIBLE_NAME,
      base::UTF8ToUTF16(user->display_email)));
}

void LoginPasswordView::SetFocusEnabledForChildViews(bool enable) {
  auto behavior = enable ? FocusBehavior::ALWAYS : FocusBehavior::NEVER;
  textfield_->SetFocusBehavior(behavior);
  submit_button_->SetFocusBehavior(behavior);
}

void LoginPasswordView::Clear() {
  textfield_->SetText(base::string16());
}

void LoginPasswordView::AppendNumber(int value) {
  textfield_->SetText(textfield_->text() + base::IntToString16(value));
}

void LoginPasswordView::Backspace() {
  // Instead of just adjusting textfield_ text directly, fire a backspace key
  // event as this handles the various edge cases (ie, selected text).

  // views::Textfield::OnKeyPressed is private, so we call it via views::View.
  auto* view = static_cast<views::View*>(textfield_);
  view->OnKeyPressed(ui::KeyEvent(ui::ET_KEY_PRESSED, ui::VKEY_BACK,
                                  ui::DomCode::BACKSPACE, ui::EF_NONE));
  view->OnKeyPressed(ui::KeyEvent(ui::ET_KEY_RELEASED, ui::VKEY_BACK,
                                  ui::DomCode::BACKSPACE, ui::EF_NONE));
}

void LoginPasswordView::Submit() {}

const char* LoginPasswordView::GetClassName() const {
  return kLoginPasswordViewName;
}

gfx::Size LoginPasswordView::CalculatePreferredSize() const {
  gfx::Size size = views::View::CalculatePreferredSize();
  size.set_width(kPasswordTotalWidthDp);
  return size;
}

void LoginPasswordView::RequestFocus() {
  textfield_->RequestFocus();
}

bool LoginPasswordView::OnKeyPressed(const ui::KeyEvent& event) {
  if (event.key_code() == ui::KeyboardCode::VKEY_RETURN) {
    SubmitPassword();
    return true;
  }

  return false;
}

void LoginPasswordView::ButtonPressed(views::Button* sender,
                                      const ui::Event& event) {
  DCHECK_EQ(sender, submit_button_);
  SubmitPassword();
}

void LoginPasswordView::SubmitPassword() {
  on_submit_.Run(textfield_->text());
  Clear();
}

}  // namespace ash
