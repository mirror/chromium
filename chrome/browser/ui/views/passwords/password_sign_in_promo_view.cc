// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/passwords/password_sign_in_promo_view.h"

#include "base/metrics/user_metrics.h"
#include "build/buildflag.h"
#include "chrome/browser/signin/account_consistency_mode_manager.h"
#include "chrome/browser/ui/passwords/manage_passwords_bubble_model.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/browser/ui/views/harmony/chrome_typography.h"
#include "chrome/browser/ui/views/sync/dice_bubble_sync_promo_view.h"
#include "chrome/grit/generated_resources.h"
#include "components/signin/core/browser/account_info.h"
#include "components/signin/core/browser/signin_features.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/layout/fill_layout.h"

// Delegate for the personalized sync promo view used when desktop identity
// consistency is enabled. Handles sign in and enable sync for existing
// accounts requests.
class PasswordBubbleSyncPromoDelegate : public BubbleSyncPromoDelegate {
 public:
  explicit PasswordBubbleSyncPromoDelegate(ManagePasswordsBubbleModel* model)
      : model_(model) {
    DCHECK(model_);
  }
  ~PasswordBubbleSyncPromoDelegate() override = default;

  void OnEnableSync(const AccountInfo& account) override {
    model_->OnSignInToChromeClicked(account);
  };

 private:
  ManagePasswordsBubbleModel* model_;

  DISALLOW_COPY_AND_ASSIGN(PasswordBubbleSyncPromoDelegate);
};

PasswordSignInPromoView::PasswordSignInPromoView(
    ManagePasswordsBubbleModel* model)
    : model_(model) {
  DCHECK(model_);
  base::RecordAction(
      base::UserMetricsAction("Signin_Impression_FromPasswordBubble"));

#if BUILDFLAG(ENABLE_DICE_SUPPORT)
  Profile* profile = model_->GetProfile();
  if (AccountConsistencyModeManager::IsDiceEnabledForProfile(profile)) {
    dice_sync_promo_delegate_ =
        std::make_unique<PasswordBubbleSyncPromoDelegate>(model_);
    SetLayoutManager(std::make_unique<views::FillLayout>());
    AddChildView(new DiceBubbleSyncPromoView(
        profile, dice_sync_promo_delegate_.get(),
        IDS_PASSWORD_MANAGER_DICE_PROMO_SIGNIN_MESSAGE,
        IDS_PASSWORD_MANAGER_DICE_PROMO_SYNC_MESSAGE));
    // const gfx::Insets content_insets =
    //  ChromeLayoutProvider::Get()->GetDialogInsetsForContentType(views::TEXT,
    //  views::TEXT);
    // set_margins(gfx::Insets(content_insets.top(), content_insets.left(),
    //  ChromeLayoutProvider::Get()->GetInsetsMetric(views::INSETS_DIALOG).bottom(),
    //  content_insets.right()));
  }
#endif
}

PasswordSignInPromoView::~PasswordSignInPromoView() = default;

bool PasswordSignInPromoView::Accept() {
  DCHECK(!dice_sync_promo_delegate_);
  model_->OnSignInToChromeClicked(AccountInfo());
  return true;
}

bool PasswordSignInPromoView::Cancel() {
  DCHECK(!dice_sync_promo_delegate_);
  model_->OnSkipSignInClicked();
  return true;
}

int PasswordSignInPromoView::GetDialogButtons() const {
  if (dice_sync_promo_delegate_) {
    // The desktop identity consistency sync promo has its own promo message
    // and button (it does not reuse the ManagePasswordPendingView's dialog
    // buttons).
    return ui::DIALOG_BUTTON_NONE;
  }
  return ui::DIALOG_BUTTON_CANCEL;
}

base::string16 PasswordSignInPromoView::GetDialogButtonLabel(
    ui::DialogButton button) const {
  DCHECK(!dice_sync_promo_delegate_);
  return l10n_util::GetStringUTF16(
      button == ui::DIALOG_BUTTON_OK
          ? IDS_PASSWORD_MANAGER_SIGNIN_PROMO_SIGN_IN
          : IDS_PASSWORD_MANAGER_SIGNIN_PROMO_NO_THANKS);
}
