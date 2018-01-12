// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/passwords/manage_password_save_confirmation_view.h"

#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/browser/ui/views/harmony/chrome_typography.h"
#include "chrome/browser/ui/views/passwords/manage_passwords_bubble_view.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/layout/fill_layout.h"

ManagePasswordSaveConfirmationView::ManagePasswordSaveConfirmationView(
    content::WebContents* web_contents,
    views::View* anchor_view,
    const gfx::Point& anchor_point,
    DisplayReason reason)
    : ManagePasswordsBubbleDelegateViewBase(web_contents,
                                            anchor_view,
                                            anchor_point,
                                            reason) {
  SetLayoutManager(std::make_unique<views::FillLayout>());

  auto label = std::make_unique<views::Label>(
      l10n_util::GetStringUTF16(IDS_MANAGE_PASSWORDS_CONFIRM_GENERATED_TEXT),
      CONTEXT_BODY_TEXT_LARGE, STYLE_SECONDARY);
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetMultiLine(true);

  AddChildView(label.release());
}

ManagePasswordSaveConfirmationView::~ManagePasswordSaveConfirmationView() =
    default;

int ManagePasswordSaveConfirmationView::GetDialogButtons() const {
  // This dialog uses a cancel button as it's styled as a secondary button.
  return ui::DIALOG_BUTTON_OK;
}

base::string16 ManagePasswordSaveConfirmationView::GetDialogButtonLabel(
    ui::DialogButton button) const {
  DCHECK_EQ(ui::DIALOG_BUTTON_OK, button);
  return l10n_util::GetStringUTF16(
      IDS_MANAGE_PASSWORDS_CONFIRM_GENERATED_VIEW_SAVED_PASSWORDS_BUTTON);
}

bool ManagePasswordSaveConfirmationView::ShouldShowCloseButton() const {
  return true;
}

bool ManagePasswordSaveConfirmationView::Accept() {
  model()->OnNavigateToPasswordManagerAccountDashboardClicked();
  return true;
}

bool ManagePasswordSaveConfirmationView::Close() {
  return true;
}

gfx::Size ManagePasswordSaveConfirmationView::CalculatePreferredSize() const {
  const int width = ChromeLayoutProvider::Get()->GetDistanceMetric(
                        DISTANCE_BUBBLE_PREFERRED_WIDTH) -
                    margins().width();
  return gfx::Size(width, GetHeightForWidth(width));
}
