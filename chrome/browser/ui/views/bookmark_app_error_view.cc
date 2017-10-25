// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/bookmark_app_error_view.h"

#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"


BookmarkAppErrorView::BookmarkAppErrorView(content::WebContents* web_contents)
    : chrome::BookmarkAppError(web_contents) {
  SetBackground(views::CreateThemedSolidBackground(
      this, ui::NativeTheme::kColorId_DialogBackground));

  auto* layout = new views::BoxLayout(views::BoxLayout::kVertical);
  layout->set_cross_axis_alignment(views::BoxLayout::CROSS_AXIS_ALIGNMENT_CENTER);
  layout->set_main_axis_alignment(views::BoxLayout::MAIN_AXIS_ALIGNMENT_CENTER);
  SetLayoutManager(layout);

  title_ = new views::Label(base::UTF8ToUTF16("There is a problem with the app."));
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  title_->SetFontList(rb.GetFontList(ui::ResourceBundle::LargeFont));
  AddChildView(title_);

  empty_ = new views::Label(base::UTF8ToUTF16(""));
  AddChildView(empty_);

  action_button_ = views::MdTextButton::CreateSecondaryUiBlueButton(
      this, base::UTF8ToUTF16("Open in chrome"));
  AddChildView(action_button_);

  views::Widget::InitParams bookmark_app_error_params(
      views::Widget::InitParams::TYPE_CONTROL);

  // It is not possible to create a native_widget_win that has no parent in
  // and later re-parent it.
  bookmark_app_error_params.parent = web_contents->GetNativeView();

  set_owned_by_client();

  views::Widget* bookmark_app_error = new views::Widget();
  bookmark_app_error->Init(bookmark_app_error_params);
  bookmark_app_error->SetContentsView(this);

  views::Widget::ReparentNativeView(bookmark_app_error->GetNativeView(),
                                    web_contents->GetNativeView());
  gfx::Rect bounds = web_contents->GetContainerBounds();
  bookmark_app_error->SetBounds(gfx::Rect(bounds.size()));
}

BookmarkAppErrorView::~BookmarkAppErrorView() {
  if (GetWidget())
    GetWidget()->Close();
}

void BookmarkAppErrorView::ButtonPressed(views::Button* sender,
                                         const ui::Event& event) {
  OpenInChrome();
}

namespace chrome {

BookmarkAppError* BookmarkAppError::Create(content::WebContents* web_contents) {
  return new BookmarkAppErrorView(web_contents);

}

}  // namespace chrome
