// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/chromeos/ksv/views/ksv_search_box_view.h"

#include "base/strings/utf_string_conversions.h"
#include "ui/chromeos/ksv/vector_icons/vector_icons.h"
#include "ui/chromeos/search_box/search_box_view_delegate.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/border.h"
#include "ui/views/controls/textfield/textfield.h"

namespace keyboard_shortcut_viewer {

namespace {

constexpr SkColor kDefaultBackgroundColor =
    SkColorSetARGB(0x1E, 0x80, 0x86, 0x8B);

// Preferred height of search box.
constexpr int kPreferredHeight = 32;
// Preferred width of search box.
constexpr int kPreferredWidth = 704;

// Border corner radius of the search box.
constexpr int kBorderCornerRadius = 32;
constexpr int kBorderThichness = 2;

}  // namespace

KSVSearchBoxView::KSVSearchBoxView(search_box::SearchBoxViewDelegate* delegate)
    : search_box::SearchBoxViewBase(delegate) {
  SetSearchBoxBackgroundCornerRadius(kBorderCornerRadius);
  SetSearchBoxBackgroundColor(kDefaultBackgroundColor);
  search_box()->SetBackgroundColor(SK_ColorTRANSPARENT);
  search_box()->set_placeholder_text(base::ASCIIToUTF16("Search for shortcut"));
  search_box()->set_placeholder_text_draw_flags(gfx::Canvas::TEXT_ALIGN_CENTER);
  SetSearchIconImage(gfx::CreateVectorIcon(kKsvSearchBarIcon, SK_ColorBLUE));
}

gfx::Size KSVSearchBoxView::CalculatePreferredSize() const {
  return gfx::Size(kPreferredWidth, kPreferredHeight);
}

void KSVSearchBoxView::ModelChanged() {}
void KSVSearchBoxView::UpdateKeyboardVisibility() {}
void KSVSearchBoxView::UpdateModel(bool initiated_by_user) {}
void KSVSearchBoxView::UpdateSearchIcon() {}

void KSVSearchBoxView::UpdateSearchBoxBorder() {
  if (search_box()->HasFocus()) {
    SetBorder(views::CreateRoundedRectBorder(
        kBorderThichness, kBorderCornerRadius, SK_ColorBLUE));
    return;
  }
  SetBorder(views::CreateRoundedRectBorder(kBorderThichness,
                                           kBorderCornerRadius, SK_ColorGRAY));
}

void KSVSearchBoxView::SetupCloseButton() {
  views::ImageButton* close = close_button();
  close->SetImage(views::ImageButton::STATE_NORMAL,
                  gfx::CreateVectorIcon(kKsvSearchCloseIcon, SK_ColorBLUE));
  close->SetImageAlignment(views::ImageButton::ALIGN_CENTER,
                           views::ImageButton::ALIGN_MIDDLE);
  close->SetVisible(false);
}

void KSVSearchBoxView::SetupBackButton() {
  views::ImageButton* back = back_button();
  back->SetImage(views::ImageButton::STATE_NORMAL,
                 gfx::CreateVectorIcon(kKsvSearchBackIcon, SK_ColorBLUE));
  back->SetImageAlignment(views::ImageButton::ALIGN_CENTER,
                          views::ImageButton::ALIGN_MIDDLE);
  back->SetVisible(false);
}

}  // namespace keyboard_shortcut_viewer
