// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_PASSWORDS_MANAGE_PASSWORD_ITEMS_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_PASSWORDS_MANAGE_PASSWORD_ITEMS_VIEW_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "chrome/browser/ui/views/location_bar/location_bar_bubble_delegate_view.h"
#include "components/autofill/core/common/password_form.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"

namespace views {
class GridLayout;
class Textfield;
class Label;
}  // namespace views

class ManagePasswordsBubbleModel;

// Standalone functions for creating username and password views.
std::unique_ptr<views::Label> CreateUsernameLabel(
    const autofill::PasswordForm& form);
std::unique_ptr<views::Label> CreatePasswordLabel(
    const autofill::PasswordForm& form,
    bool is_password_visible);
std::unique_ptr<views::Textfield> CreateUsernameEditable(
    const autofill::PasswordForm& form);

// A custom view of individual credentials. The view is represented as a table
// where each row can offer the user the ability to undo a deletion action.
class ManagePasswordItemsView : public views::View {
 public:
  ManagePasswordItemsView(
      ManagePasswordsBubbleModel* manage_passwords_bubble_model,
      const std::vector<autofill::PasswordForm>* password_forms,
      int bubble_width);

 private:
  class PasswordFormRow;

  ~ManagePasswordItemsView() override;

  void AddRows();
  void NotifyPasswordFormStatusChanged(
      const autofill::PasswordForm& password_form, bool deleted);

  // Changes the views according to the state of |password_forms_rows_|.
  void Refresh();

  std::vector<std::unique_ptr<PasswordFormRow>> password_forms_rows_;
  ManagePasswordsBubbleModel* model_;
  const int bubble_width_;

  DISALLOW_COPY_AND_ASSIGN(ManagePasswordItemsView);
};

class ManagePasswordItemsBubbleView : public LocationBarBubbleDelegateView,
                                      public views::ButtonListener {
 public:
  ManagePasswordItemsBubbleView(
      content::WebContents* web_contents,
      views::View* anchor_view,
      LocationBarBubbleDelegateView** global_bubble_view,
      content::WebContents** global_web_contents,
      std::unique_ptr<ManagePasswordsBubbleModel> model);
  ~ManagePasswordItemsBubbleView() override;

 private:
  class PasswordRow : public views::ButtonListener {
   public:
    PasswordRow(ManagePasswordItemsBubbleView* parent,
                const autofill::PasswordForm* password_form);

    void AddToLayout(views::GridLayout* layout);

   private:
    void ButtonPressed(views::Button* sender, const ui::Event& event) override;

    ManagePasswordItemsBubbleView* const parent_;
    const autofill::PasswordForm* const password_form_;
    bool deleted_ = false;
  };

  void NotifyPasswordFormStatusChanged(
      const autofill::PasswordForm& password_form,
      bool deleted);
  void Refresh();

  base::string16 GetWindowTitle() const override;
  View* CreateExtraView() override;
  int GetDialogButtons() const override;
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;
  bool ShouldShowCloseButton() const override;
  gfx::Size CalculatePreferredSize() const override;

  LocationBarBubbleDelegateView** const global_bubble_view_;
  content::WebContents** const global_web_contents_;
  std::unique_ptr<ManagePasswordsBubbleModel> model_;
  std::unique_ptr<WebContentMouseHandler> mouse_handler_;
  std::vector<PasswordRow> password_rows_;
};

#endif  // CHROME_BROWSER_UI_VIEWS_PASSWORDS_MANAGE_PASSWORD_ITEMS_VIEW_H_
