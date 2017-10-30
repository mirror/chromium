// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/passwords/manage_password_items_view.h"

#include <numeric>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/ui/passwords/manage_passwords_bubble_model.h"
#include "chrome/browser/ui/passwords/manage_passwords_view_utils.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/browser/ui/views/harmony/chrome_typography.h"
#include "chrome/grit/generated_resources.h"
#include "components/password_manager/core/common/password_manager_ui.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/range/range.h"
#include "ui/resources/grit/ui_resources.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/image_button_factory.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"
#include "ui/views/controls/link_listener.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/grid_layout.h"

namespace {

// These different column sets correspond to the three different types of rows
// that can be shown in the manage-password-items dialog. They all allocate
// space differently.
enum ColumnSetType {
  // Column set for password credentials.
  PASSWORD_COLUMN_SET,
  // Column set for undoing credential deletion.
  UNDO_COLUMN_SET
};

void BuildColumnSet(views::GridLayout* layout, ColumnSetType style_id) {
  DCHECK(!layout->GetColumnSet(style_id));
  views::ColumnSet* column_set = layout->AddColumnSet(style_id);
  // Passwords are split 60/40 (6:4) as the username is more important
  // than obscured password digits. Otherwise two columns are 50/50 (1:1).
  constexpr int kFirstColumnWeight = 6;
  constexpr int kSecondColumnWeight = 4;
  const int between_column_padding =
      ChromeLayoutProvider::Get()->GetDistanceMetric(
          views::DISTANCE_RELATED_CONTROL_HORIZONTAL);
  column_set->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL,
                        kFirstColumnWeight, views::GridLayout::FIXED, 0, 0);

  if (style_id == PASSWORD_COLUMN_SET) {
    column_set->AddPaddingColumn(0, between_column_padding);
    column_set->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL,
                          kSecondColumnWeight, views::GridLayout::FIXED, 0, 0);
  }
  // All rows end with a trailing column for the undo/trash button.
  column_set->AddPaddingColumn(0, between_column_padding);
  column_set->AddColumn(views::GridLayout::TRAILING, views::GridLayout::FILL, 0,
                        views::GridLayout::USE_PREF, 0, 0);
}

void StartRow(views::GridLayout* layout, ColumnSetType style_id) {
  if (!layout->GetColumnSet(style_id))
    BuildColumnSet(layout, style_id);
  layout->StartRow(0, style_id);
}

constexpr int kDeleteButtonTag = 1;
constexpr int kUndoButtonTag = 2;

std::unique_ptr<views::ImageButton> CreateDeleteButton(
    views::ButtonListener* listener,
    const base::string16 username) {
  std::unique_ptr<views::ImageButton> button;
  button.reset(views::CreateVectorImageButton(listener));
  views::SetImageFromVectorIcon(button.get(), kTrashCanIcon);
  button->SetFocusForPlatform();
  button->SetTooltipText(
      l10n_util::GetStringFUTF16(IDS_MANAGE_PASSWORDS_DELETE, username));
  button->set_tag(kDeleteButtonTag);
  return button;
}

std::unique_ptr<views::LabelButton> CreateUndoButton(
    views::ButtonListener* listener,
    const base::string16 username) {
  std::unique_ptr<views::LabelButton> undo_button(
      views::MdTextButton::CreateSecondaryUiButton(
          listener, l10n_util::GetStringUTF16(IDS_MANAGE_PASSWORDS_UNDO)));
  undo_button->set_tag(kUndoButtonTag);
  undo_button->SetFocusForPlatform();
  undo_button->SetTooltipText(
      l10n_util::GetStringFUTF16(IDS_MANAGE_PASSWORDS_UNDO_TOOLTIP, username));
  return undo_button;
}

}  // namespace

std::unique_ptr<views::Label> CreateUsernameLabel(
    const autofill::PasswordForm& form) {
  auto label = base::MakeUnique<views::Label>(
      GetDisplayUsername(form), CONTEXT_BODY_TEXT_LARGE, STYLE_SECONDARY);
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  return label;
}

std::unique_ptr<views::Textfield> CreateUsernameEditable(
    const autofill::PasswordForm& form) {
  auto editable = base::MakeUnique<views::Textfield>();
  editable->SetText(form.username_value);
  editable->SetAccessibleName(
      l10n_util::GetStringUTF16(IDS_PASSWORD_MANAGER_USERNAME_LABEL));
  // In case of long username, ensure that the beginning of value is visible.
  editable->SelectRange(gfx::Range(0));
  return editable;
}

std::unique_ptr<views::Label> CreatePasswordLabel(
    const autofill::PasswordForm& form,
    bool is_password_visible) {
  base::string16 text =
      form.federation_origin.unique()
          ? form.password_value
          : l10n_util::GetStringFUTF16(
                IDS_PASSWORDS_VIA_FEDERATION,
                base::UTF8ToUTF16(form.federation_origin.host()));
  auto label = base::MakeUnique<views::Label>(text, CONTEXT_BODY_TEXT_LARGE,
                                              STYLE_SECONDARY);
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  if (form.federation_origin.unique() && !is_password_visible)
    label->SetObscured(true);
  if (!form.federation_origin.unique())
    label->SetElideBehavior(gfx::ELIDE_HEAD);
  label->SetFocusBehavior(views::View::FocusBehavior::ACCESSIBLE_ONLY);
  return label;
}

ManagePasswordItemsView::PasswordRow::PasswordRow(
    ManagePasswordItemsView* parent,
    const autofill::PasswordForm* password_form)
    : parent_(parent), password_form_(password_form) {}

void ManagePasswordItemsView::PasswordRow::AddToLayout(
    views::GridLayout* layout) {
  if (deleted_) {
    AddUndoRow(layout);
  } else {
    AddPasswordRow(layout);
  }
}

void ManagePasswordItemsView::PasswordRow::AddUndoRow(
    views::GridLayout* layout) {
  std::unique_ptr<views::Label> text = base::MakeUnique<views::Label>(
      l10n_util::GetStringUTF16(IDS_MANAGE_PASSWORDS_DELETED),
      CONTEXT_BODY_TEXT_LARGE);
  text->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  std::unique_ptr<views::LabelButton> undo_button =
      CreateUndoButton(this, GetDisplayUsername(*password_form_));

  StartRow(layout, UNDO_COLUMN_SET);
  layout->AddView(text.release(), 1, 1, views::GridLayout::FILL,
                  views::GridLayout::FILL);
  layout->AddView(undo_button.release(), 1, 1, views::GridLayout::FILL,
                  views::GridLayout::FILL);
}

void ManagePasswordItemsView::PasswordRow::AddPasswordRow(
    views::GridLayout* layout) {
  std::unique_ptr<views::Label> username_label =
      CreateUsernameLabel(*password_form_);
  std::unique_ptr<views::Label> password_label =
      CreatePasswordLabel(*password_form_, false);
  std::unique_ptr<views::ImageButton> delete_button =
      CreateDeleteButton(this, GetDisplayUsername(*password_form_));
  StartRow(layout, PASSWORD_COLUMN_SET);
  layout->AddView(username_label.release(), 1, 1, views::GridLayout::FILL,
                  views::GridLayout::FILL);
  layout->AddView(password_label.release(), 1, 1, views::GridLayout::FILL,
                  views::GridLayout::FILL);
  layout->AddView(delete_button.release(), 1, 1, views::GridLayout::TRAILING,
                  views::GridLayout::FILL);
}

void ManagePasswordItemsView::PasswordRow::ButtonPressed(
    views::Button* sender,
    const ui::Event& event) {
  DCHECK(sender->tag() == kDeleteButtonTag || sender->tag() == kUndoButtonTag);
  deleted_ = sender->tag() == kDeleteButtonTag;
  parent_->NotifyPasswordFormAction(
      *password_form_, deleted_ ? ManagePasswordsBubbleModel::REMOVE_PASSWORD
                                : ManagePasswordsBubbleModel::ADD_PASSWORD);
}

ManagePasswordItemsView::ManagePasswordItemsView(
    content::WebContents* web_contents,
    views::View* anchor_view,
    LocationBarBubbleDelegateView** global_bubble_view,
    std::unique_ptr<ManagePasswordsBubbleModel> model)
    : LocationBarBubbleDelegateView(anchor_view, gfx::Point(), web_contents),
      global_bubble_view_(global_bubble_view),
      model_(std::move(model)),
      mouse_handler_(
          std::make_unique<WebContentMouseHandler>(this,
                                                   model_->GetWebContents())) {
  DCHECK_EQ(password_manager::ui::MANAGE_STATE, model_->state());

  *global_bubble_view_ = this;

  if (model_->local_credentials().empty()) {
    views::Label* no_passwords_label = new views::Label(
        l10n_util::GetStringUTF16(IDS_MANAGE_PASSWORDS_NO_PASSWORDS),
        CONTEXT_BODY_TEXT_SMALL);
    no_passwords_label->SetMultiLine(true);
    no_passwords_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    AddChildView(no_passwords_label);
  } else {
    for (auto& password_form : model_->local_credentials())
      password_rows_.emplace_back(PasswordRow(this, &password_form));

    RecreateLayout();
  }
}

ManagePasswordItemsView::~ManagePasswordItemsView() {
  if (*global_bubble_view_ == this)
    *global_bubble_view_ = nullptr;
}

void ManagePasswordItemsView::RecreateLayout() {
  // This method should only be used when we have password rows, otherwise the
  // dialog should only show the empty label which doesn't need to be recreated.
  DCHECK(!model_->local_credentials().empty());

  RemoveAllChildViews(true);

  views::GridLayout* grid_layout = views::GridLayout::CreateAndInstall(this);
  SetLayoutManager(grid_layout);

  const int vertical_padding = ChromeLayoutProvider::Get()->GetDistanceMetric(
      DISTANCE_CONTROL_LIST_VERTICAL);
  bool first_row = true;
  for (PasswordRow& row : password_rows_) {
    if (!first_row)
      grid_layout->AddPaddingRow(0, vertical_padding);

    row.AddToLayout(grid_layout);
    first_row = false;
  }

  PreferredSizeChanged();
  if (GetBubbleFrameView())
    SizeToContents();
}

void ManagePasswordItemsView::NotifyPasswordFormAction(
    const autofill::PasswordForm& password_form,
    ManagePasswordsBubbleModel::PasswordAction action) {
  RecreateLayout();
  // After the view is consistent, notify the model that the password needs to
  // be updated (either removed or put back into the store, as appropriate.
  model_->OnPasswordAction(password_form, action);
}

base::string16 ManagePasswordItemsView::GetWindowTitle() const {
  return model_->title();
}

views::View* ManagePasswordItemsView::CreateExtraView() {
  return views::MdTextButton::CreateSecondaryUiButton(
      this, l10n_util::GetStringUTF16(IDS_MANAGE_PASSWORDS_BUBBLE_LINK));
}

int ManagePasswordItemsView::GetDialogButtons() const {
  return ui::DIALOG_BUTTON_OK;
}

bool ManagePasswordItemsView::ShouldShowCloseButton() const {
  return true;
}

gfx::Size ManagePasswordItemsView::CalculatePreferredSize() const {
  const int width = ChromeLayoutProvider::Get()->GetDistanceMetric(
                        DISTANCE_BUBBLE_PREFERRED_WIDTH) -
                    margins().width();
  return gfx::Size(width, GetHeightForWidth(width));
}

void ManagePasswordItemsView::ButtonPressed(views::Button* sender,
                                            const ui::Event& event) {
  model_->OnManageClicked();
  CloseBubble();
}
