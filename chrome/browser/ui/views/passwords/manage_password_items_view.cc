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

enum NewColumnSet {
  PASSWORD_FEDERATION_COLUMN_SET,
  PASSWORD_SIMPLE_COLUMN_SET,
  UNDO_ROW_COLUMN_SET
};

void BuildColumnSet(views::GridLayout* layout, NewColumnSet id) {
  DCHECK(!layout->GetColumnSet(id));
  views::ColumnSet* column_set = layout->AddColumnSet(id);
  // First two columns are 50/50 (1:1) when not doing simple password. Simple
  // passwords are split 70/30 (7:3) as the username is more important than
  // obscured password digits.
  const int column_divider = ChromeLayoutProvider::Get()->GetDistanceMetric(
      views::DISTANCE_RELATED_CONTROL_HORIZONTAL);
  const int first_column_weight = id == PASSWORD_SIMPLE_COLUMN_SET ? 7 : 1;
  column_set->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL,
                        first_column_weight, views::GridLayout::FIXED, 0, 0);

  if (id != UNDO_ROW_COLUMN_SET) {
    column_set->AddPaddingColumn(0, column_divider);
    const int second_column_weight = id == PASSWORD_SIMPLE_COLUMN_SET ? 3 : 1;
    column_set->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL,
                          second_column_weight, views::GridLayout::FIXED, 0, 0);
  }
  // All rows end with a trailing column for the undo button / trash icon.
  column_set->AddPaddingColumn(0, column_divider);
  column_set->AddColumn(views::GridLayout::TRAILING, views::GridLayout::FILL, 0,
                        views::GridLayout::USE_PREF, 0, 0);
}

enum ButtonTag { kDeleteButton, kUndoButton };

enum ColumnSets {
  ONE_COLUMN_SET,
  TWO_COLUMN_SET,
  THREE_COLUMN_SET
};

void BuildColumnSetIfNeeded(views::GridLayout* layout, int column_set_id) {
  DCHECK(!ChromeLayoutProvider::Get()->IsHarmonyMode());
  if (layout->GetColumnSet(column_set_id))
    return;
  views::ColumnSet* column_set = layout->AddColumnSet(column_set_id);

  // The username/"Deleted!"/Border field.
  column_set->AddColumn(views::GridLayout::FILL,
                        views::GridLayout::FILL,
                        1,
                        views::GridLayout::USE_PREF,
                        0,
                        0);

  const int column_divider = ChromeLayoutProvider::Get()->GetDistanceMetric(
      views::DISTANCE_RELATED_CONTROL_HORIZONTAL);
  if (column_set_id >= TWO_COLUMN_SET) {
    // The password/"Undo!" field.
    column_set->AddPaddingColumn(0, column_divider);
    column_set->AddColumn(views::GridLayout::FILL,
                          views::GridLayout::FILL,
                          1,
                          views::GridLayout::USE_PREF,
                          0,
                          0);
  }
  // If we're in manage-mode, we need another column for the delete button.
  if (column_set_id == THREE_COLUMN_SET) {
    column_set->AddPaddingColumn(0, column_divider);
    column_set->AddColumn(views::GridLayout::TRAILING,
                          views::GridLayout::FILL,
                          0,
                          views::GridLayout::USE_PREF,
                          0,
                          0);
  }
}

std::unique_ptr<views::ImageButton> CreateDeleteButton(
    views::ButtonListener* listener) {
  std::unique_ptr<views::ImageButton> button;
  if (ChromeLayoutProvider::Get()->IsHarmonyMode()) {
    button.reset(views::CreateVectorImageButton(listener));
    views::SetImageFromVectorIcon(button.get(), kTrashCanIcon);
  } else {
    ui::ResourceBundle* rb = &ui::ResourceBundle::GetSharedInstance();
    button = base::MakeUnique<views::ImageButton>(listener);
    button->SetImage(views::ImageButton::STATE_NORMAL,
                     rb->GetImageNamed(IDR_CLOSE_2).ToImageSkia());
    button->SetImage(views::ImageButton::STATE_HOVERED,
                     rb->GetImageNamed(IDR_CLOSE_2_H).ToImageSkia());
    button->SetImage(views::ImageButton::STATE_PRESSED,
                     rb->GetImageNamed(IDR_CLOSE_2_P).ToImageSkia());
  }
  button->SetFocusForPlatform();
  button->SetTooltipText(
      l10n_util::GetStringUTF16(IDS_MANAGE_PASSWORDS_DELETE));
  return button;
}

std::unique_ptr<views::Label> CreateDeletedPasswordLabel() {
  auto text = base::MakeUnique<views::Label>(
      l10n_util::GetStringUTF16(IDS_MANAGE_PASSWORDS_DELETED),
      CONTEXT_BODY_TEXT_LARGE);
  text->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  return text;
}

std::unique_ptr<views::Link> CreateUndoLink(views::LinkListener* listener) {
  std::unique_ptr<views::Link> undo_link(
      new views::Link(l10n_util::GetStringUTF16(IDS_MANAGE_PASSWORDS_UNDO)));
  undo_link->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
  undo_link->set_listener(listener);
  undo_link->SetUnderline(false);
  return undo_link;
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

// Manage credentials: stores credentials state and adds proper row to layout
// based on credential state.
class ManagePasswordItemsView::PasswordFormRow : public views::ButtonListener,
                                                 public views::LinkListener {
 public:
  PasswordFormRow(ManagePasswordItemsView* host,
                  const autofill::PasswordForm* password_form,
                  int fixed_height);
  ~PasswordFormRow() override = default;

  void AddRow(views::GridLayout* layout);

  // Returns the fixed height for a row excluding padding. 0 means no fixed
  // height required.
  // In MANAGE_STATE a row may represent a credential or a deleted credential.
  // To avoid repositioning all the rows should have a fixed height.
  static int GetFixedHeight(password_manager::ui::State state);

 private:
  void AddCredentialsRow(views::GridLayout* layout);
  void AddUndoRow(views::GridLayout* layout);

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;
  // views::LinkListener:
  void LinkClicked(views::Link* source, int event_flags) override;

  void ResetControls();

  ManagePasswordItemsView* host_;
  const autofill::PasswordForm* password_form_;
  // The UI elements pointers are weak and owned by their parent.
  views::Link* undo_link_;
  views::ImageButton* delete_button_;
  const int fixed_height_;
  bool deleted_;

  DISALLOW_COPY_AND_ASSIGN(PasswordFormRow);
};

ManagePasswordItemsView::PasswordFormRow::PasswordFormRow(
    ManagePasswordItemsView* host,
    const autofill::PasswordForm* password_form,
    int fixed_height)
    : host_(host),
      password_form_(password_form),
      undo_link_(nullptr),
      delete_button_(nullptr),
      fixed_height_(fixed_height),
      deleted_(false) {
  DCHECK(!ChromeLayoutProvider::Get()->IsHarmonyMode());
}

void ManagePasswordItemsView::PasswordFormRow::AddRow(
    views::GridLayout* layout) {
  if (deleted_) {
    AddUndoRow(layout);
  } else {
    AddCredentialsRow(layout);
  }
}

int ManagePasswordItemsView::PasswordFormRow::GetFixedHeight(
    password_manager::ui::State state) {
  if (state != password_manager::ui::MANAGE_STATE)
    return 0;
  std::unique_ptr<views::ImageButton> delete_button(
      CreateDeleteButton(nullptr));
  std::unique_ptr<views::Link> link(CreateUndoLink(nullptr));
  std::unique_ptr<views::Label> label(CreateDeletedPasswordLabel());
  views::View* row_views[] = {delete_button.get(), link.get(), label.get()};
  return std::accumulate(row_views, row_views + arraysize(row_views), 0,
                         [](int max_height, const views::View* view) {
    return std::max(max_height, view->GetPreferredSize().height());
  });
}

void ManagePasswordItemsView::PasswordFormRow::AddCredentialsRow(
    views::GridLayout* layout) {
  ResetControls();
  BuildColumnSetIfNeeded(layout, THREE_COLUMN_SET);
  layout->StartRow(0, THREE_COLUMN_SET);
  std::unique_ptr<views::Label> username_label(
      CreateUsernameLabel(*password_form_));
  std::unique_ptr<views::Label> password_label(
      CreatePasswordLabel(*password_form_, false));
  delete_button_ = CreateDeleteButton(this).release();
  // TODO(https://crbug.com/761767): Remove this workaround once the grid layout
  // bug is fixed.
  const int username_width = username_label->CalculatePreferredSize().width();
  const int password_width = password_label->CalculatePreferredSize().width();
  const int available_width =
      host_->bubble_width_ - delete_button_->CalculatePreferredSize().width() -
      2 * ChromeLayoutProvider::Get()->GetDistanceMetric(
              views::DISTANCE_RELATED_CONTROL_HORIZONTAL);
  if (username_width > available_width && password_width < available_width) {
    layout->AddView(username_label.release(), 1, 1, views::GridLayout::FILL,
                    views::GridLayout::FILL, available_width, fixed_height_);
    layout->AddView(password_label.release(), 1, 1, views::GridLayout::FILL,
                    views::GridLayout::FILL, 0, fixed_height_);
  } else if (username_width < available_width &&
             password_width > available_width) {
    layout->AddView(username_label.release(), 1, 1, views::GridLayout::FILL,
                    views::GridLayout::FILL, username_width, fixed_height_);
    layout->AddView(password_label.release(), 1, 1, views::GridLayout::FILL,
                    views::GridLayout::FILL, available_width - username_width,
                    fixed_height_);
  } else {
    layout->AddView(username_label.release(), 1, 1, views::GridLayout::FILL,
                    views::GridLayout::FILL, 0, fixed_height_);
    layout->AddView(password_label.release(), 1, 1, views::GridLayout::FILL,
                    views::GridLayout::FILL, 0, fixed_height_);
  }
  layout->AddView(delete_button_, 1, 1, views::GridLayout::TRAILING,
                  views::GridLayout::FILL, 0, fixed_height_);
}

void ManagePasswordItemsView::PasswordFormRow::AddUndoRow(
    views::GridLayout* layout) {
  ResetControls();
  std::unique_ptr<views::Label> text = CreateDeletedPasswordLabel();
  std::unique_ptr<views::Link> undo_link = CreateUndoLink(this);
  undo_link_ = undo_link.get();
  BuildColumnSetIfNeeded(layout, TWO_COLUMN_SET);
  layout->StartRow(0, TWO_COLUMN_SET);
  layout->AddView(text.release(), 1, 1,
                  views::GridLayout::FILL, views::GridLayout::FILL,
                  0, fixed_height_);
  layout->AddView(undo_link.release(), 1, 1,
                  views::GridLayout::FILL, views::GridLayout::FILL,
                  0, fixed_height_);
}

void ManagePasswordItemsView::PasswordFormRow::ButtonPressed(
    views::Button* sender, const ui::Event& event) {
  DCHECK_EQ(delete_button_, sender);
  deleted_ = true;
  host_->NotifyPasswordFormStatusChanged(*password_form_, deleted_);
}

void ManagePasswordItemsView::PasswordFormRow::LinkClicked(views::Link* sender,
                                                           int event_flags) {
  DCHECK_EQ(undo_link_, sender);
  deleted_ = false;
  host_->NotifyPasswordFormStatusChanged(*password_form_, deleted_);
}

void ManagePasswordItemsView::PasswordFormRow::ResetControls() {
  delete_button_ = nullptr;
  undo_link_ = nullptr;
}

// ManagePasswordItemsView
ManagePasswordItemsView::ManagePasswordItemsView(
    ManagePasswordsBubbleModel* manage_passwords_bubble_model,
    const std::vector<autofill::PasswordForm>* password_forms,
    int bubble_width)
    : model_(manage_passwords_bubble_model), bubble_width_(bubble_width) {
  // TODO(pbos): When cleaning up HarmonyMode, move this file to represent the
  // new dialog name.
  DCHECK(!ChromeLayoutProvider::Get()->IsHarmonyMode());
  DCHECK_EQ(password_manager::ui::MANAGE_STATE, model_->state());
  int fixed_height = PasswordFormRow::GetFixedHeight(model_->state());
  for (const auto& password_form : *password_forms) {
    password_forms_rows_.push_back(base::MakeUnique<PasswordFormRow>(
        this, &password_form, fixed_height));
  }
  AddRows();
}

ManagePasswordItemsView::~ManagePasswordItemsView() = default;

void ManagePasswordItemsView::AddRows() {
  const int vertical_padding = ChromeLayoutProvider::Get()->GetDistanceMetric(
      views::DISTANCE_RELATED_CONTROL_VERTICAL);
  views::GridLayout* layout = views::GridLayout::CreateAndInstall(this);
  SetLayoutManager(layout);
  for (const std::unique_ptr<PasswordFormRow>& row : password_forms_rows_) {
    if (row != password_forms_rows_[0])
      layout->AddPaddingRow(0, vertical_padding);
    row->AddRow(layout);
  }
  GetLayoutManager()->Layout(this);
}

void ManagePasswordItemsView::NotifyPasswordFormStatusChanged(
    const autofill::PasswordForm& password_form, bool deleted) {
  Refresh();
  // After the view is consistent, notify the model that the password needs to
  // be updated (either removed or put back into the store, as appropriate.
  model_->OnPasswordAction(password_form,
                           deleted
                               ? ManagePasswordsBubbleModel::REMOVE_PASSWORD
                               : ManagePasswordsBubbleModel::ADD_PASSWORD);
}

void ManagePasswordItemsView::Refresh() {
  RemoveAllChildViews(true);
  AddRows();
}

ManagePasswordItemsBubbleView::PasswordRow::PasswordRow(
    ManagePasswordItemsBubbleView* parent,
    const autofill::PasswordForm* password_form)
    : parent_(parent), password_form_(password_form) {}

void ManagePasswordItemsBubbleView::PasswordRow::AddToLayout(
    views::GridLayout* layout) {
  if (deleted_) {
    std::unique_ptr<views::Label> text = CreateDeletedPasswordLabel();
    std::unique_ptr<views::LabelButton> undo_button(
        views::MdTextButton::CreateSecondaryUiButton(
            this, l10n_util::GetStringUTF16(IDS_MANAGE_PASSWORDS_UNDO)));
    undo_button->set_tag(kUndoButton);

    layout->StartRow(0, UNDO_ROW_COLUMN_SET);
    layout->AddView(text.release(), 1, 1, views::GridLayout::FILL,
                    views::GridLayout::FILL);
    layout->AddView(undo_button.release(), 1, 1, views::GridLayout::FILL,
                    views::GridLayout::FILL);
    return;
  }

  std::unique_ptr<views::Label> username_label =
      CreateUsernameLabel(*password_form_);
  std::unique_ptr<views::Label> password_label =
      CreatePasswordLabel(*password_form_, false);
  std::unique_ptr<views::ImageButton> delete_button = CreateDeleteButton(this);
  delete_button->set_tag(kDeleteButton);
  layout->StartRow(0, password_form_->federation_origin.unique()
                          ? PASSWORD_SIMPLE_COLUMN_SET
                          : PASSWORD_FEDERATION_COLUMN_SET);
  layout->AddView(username_label.release(), 1, 1, views::GridLayout::FILL,
                  views::GridLayout::FILL);
  layout->AddView(password_label.release(), 1, 1, views::GridLayout::FILL,
                  views::GridLayout::FILL);
  layout->AddView(delete_button.release(), 1, 1, views::GridLayout::TRAILING,
                  views::GridLayout::FILL);
}

void ManagePasswordItemsBubbleView::PasswordRow::ButtonPressed(
    views::Button* sender,
    const ui::Event& event) {
  deleted_ = sender->tag() == kDeleteButton;
  parent_->NotifyPasswordFormStatusChanged(*password_form_, deleted_);
}

ManagePasswordItemsBubbleView::ManagePasswordItemsBubbleView(
    content::WebContents* web_contents,
    views::View* anchor_view,
    LocationBarBubbleDelegateView** global_bubble_view,
    content::WebContents** global_web_contents,
    std::unique_ptr<ManagePasswordsBubbleModel> model)
    : LocationBarBubbleDelegateView(anchor_view, gfx::Point(), web_contents),
      global_bubble_view_(global_bubble_view),
      global_web_contents_(global_web_contents),
      model_(std::move(model)),
      mouse_handler_(
          std::make_unique<WebContentMouseHandler>(this,
                                                   model_->GetWebContents())) {
  DCHECK_EQ(password_manager::ui::MANAGE_STATE, model_->state());

  *global_bubble_view_ = this;
  *global_web_contents_ = model_->GetWebContents();

  if (model_->local_credentials().empty()) {
    views::Label* empty_label = new views::Label(
        l10n_util::GetStringUTF16(IDS_MANAGE_PASSWORDS_NO_PASSWORDS),
        CONTEXT_DEPRECATED_SMALL);
    empty_label->SetMultiLine(true);
    empty_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    AddChildView(empty_label);
  } else {
    for (auto& password_form : model_->local_credentials()) {
      password_rows_.emplace_back(PasswordRow(this, &password_form));
    }

    Refresh();
    Refresh();
  }
}

ManagePasswordItemsBubbleView::~ManagePasswordItemsBubbleView() {
  if (*global_bubble_view_ == this) {
    *global_bubble_view_ = nullptr;
    *global_web_contents_ = nullptr;
  }
}

void ManagePasswordItemsBubbleView::Refresh() {
  // This method should only be used when we have password rows, otherwise the
  // dialog should only show the empty label.
  DCHECK(!model_->local_credentials().empty());

  RemoveAllChildViews(true);

  views::GridLayout* grid_layout = views::GridLayout::CreateAndInstall(this);

  BuildColumnSet(grid_layout, PASSWORD_FEDERATION_COLUMN_SET);
  BuildColumnSet(grid_layout, PASSWORD_SIMPLE_COLUMN_SET);
  BuildColumnSet(grid_layout, UNDO_ROW_COLUMN_SET);

  SetLayoutManager(grid_layout);

  const int vertical_padding = ChromeLayoutProvider::Get()->GetDistanceMetric(
      views::DISTANCE_RELATED_CONTROL_VERTICAL);
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

void ManagePasswordItemsBubbleView::NotifyPasswordFormStatusChanged(
    const autofill::PasswordForm& password_form,
    bool deleted) {
  Refresh();
  // After the view is consistent, notify the model that the password needs to
  // be updated (either removed or put back into the store, as appropriate.
  model_->OnPasswordAction(password_form,
                           deleted ? ManagePasswordsBubbleModel::REMOVE_PASSWORD
                                   : ManagePasswordsBubbleModel::ADD_PASSWORD);
}

base::string16 ManagePasswordItemsBubbleView::GetWindowTitle() const {
  return model_->title();
}

views::View* ManagePasswordItemsBubbleView::CreateExtraView() {
  return views::MdTextButton::CreateSecondaryUiButton(
      this, l10n_util::GetStringUTF16(IDS_MANAGE_PASSWORDS_BUBBLE_LINK));
}

int ManagePasswordItemsBubbleView::GetDialogButtons() const {
  return ui::DIALOG_BUTTON_OK;
}

bool ManagePasswordItemsBubbleView::ShouldShowCloseButton() const {
  return true;
}

gfx::Size ManagePasswordItemsBubbleView::CalculatePreferredSize() const {
  int width = ChromeLayoutProvider::Get()->GetDistanceMetric(
                  DISTANCE_BUBBLE_PREFERRED_WIDTH) -
              margins().width();
  return gfx::Size(width, GetHeightForWidth(width));
}

void ManagePasswordItemsBubbleView::ButtonPressed(views::Button* sender,
                                                  const ui::Event& event) {
  model_->OnManageLinkClicked();
  CloseBubble();
}
