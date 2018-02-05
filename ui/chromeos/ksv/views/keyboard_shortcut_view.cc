// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/chromeos/ksv/views/keyboard_shortcut_view.h"

#include "base/i18n/string_search.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/chromeos/ksv/keyboard_shortcut_viewer_metadata.h"
#include "ui/chromeos/ksv/vector_icons/vector_icons.h"
#include "ui/chromeos/ksv/views/keyboard_shortcut_item_list_view.h"
#include "ui/chromeos/ksv/views/keyboard_shortcut_item_view.h"
#include "ui/chromeos/ksv/views/ksv_search_box_view.h"
#include "ui/chromeos/search_box/search_box_view_base.h"
#include "ui/chromeos/strings/grit/ui_chromeos_strings.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/background.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/controls/tabbed_pane/tabbed_pane.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"

namespace keyboard_shortcut_viewer {

namespace {

KeyboardShortcutView* g_ksv_view = nullptr;

constexpr int kKSVWindowWidth = 768;

constexpr int kKSVWindowHeight = 512;

constexpr int kSearchBoxTopPadding = 8;

constexpr int kSearchBoxBottomPadding = 16;

constexpr int kSearchIllustrationIconSize = 150;

void SetupSearchIllustrationView(views::View* illustration_view,
                                 const gfx::VectorIcon& icon,
                                 int message_id) {
  illustration_view->set_owned_by_client();
  views::BoxLayout* layout = illustration_view->SetLayoutManager(
      std::make_unique<views::BoxLayout>(views::BoxLayout::kVertical));
  layout->set_main_axis_alignment(views::BoxLayout::MAIN_AXIS_ALIGNMENT_CENTER);
  views::ImageView* image_view = new views::ImageView();
  image_view->SetImage(gfx::CreateVectorIcon(icon, SK_ColorGRAY));
  image_view->SetImageSize(
      gfx::Size(kSearchIllustrationIconSize, kSearchIllustrationIconSize));
  illustration_view->AddChildView(image_view);
  illustration_view->AddChildView(
      new views::Label(l10n_util::GetStringUTF16(message_id)));
}

}  // namespace

KeyboardShortcutView::~KeyboardShortcutView() {
  DCHECK_EQ(g_ksv_view, this);
  g_ksv_view = nullptr;
}

// static
views::Widget* KeyboardShortcutView::Show(gfx::NativeWindow context) {
  if (g_ksv_view) {
    // If there is a KeyboardShortcutView window open already, just activate it.
    g_ksv_view->GetWidget()->Activate();
  } else {
    views::Widget::CreateWindowWithContextAndBounds(
        new KeyboardShortcutView(), context,
        gfx::Rect(0, 0, kKSVWindowWidth, kKSVWindowHeight));
    g_ksv_view->GetWidget()->Show();
  }
  return g_ksv_view->GetWidget();
}

KeyboardShortcutView::KeyboardShortcutView() {
  DCHECK_EQ(g_ksv_view, nullptr);
  g_ksv_view = this;

  // Default background is transparent.
  SetBackground(views::CreateSolidBackground(SK_ColorWHITE));
  InitViews();
}

void KeyboardShortcutView::InitViews() {
  // Init search box view.
  search_box_view_ = new KSVSearchBoxView(this);
  search_box_view_->Init();
  AddChildView(search_box_view_);

  // Init start searching illustration view.
  search_start_view_ = std::make_unique<views::View>();
  SetupSearchIllustrationView(search_start_view_.get(), kKsvSearchStartIcon,
                              IDS_KSV_SEARCH_START);

  // Init no search result illustration view.
  search_no_result_view_ = std::make_unique<views::View>();
  SetupSearchIllustrationView(search_no_result_view_.get(),
                              kKsvSearchNoResultIcon, IDS_KSV_SEARCH_NO_RESULT);

  // Init search results container view.
  search_results_container_ = new views::View();
  search_results_container_->SetLayoutManager(
      std::make_unique<views::FillLayout>());
  search_results_container_->AddChildView(search_start_view_.get());
  search_results_container_->SetVisible(false);
  AddChildView(search_results_container_);

  // Init views of KeyboardShortcutItemView.
  for (const auto& item : GetKeyboardShortcutItemList()) {
    for (auto category : item.categories) {
      shortcut_views_by_category_[category].emplace_back(
          new KeyboardShortcutItemView(item));
      auto item_view_for_search =
          std::make_unique<KeyboardShortcutItemView>(item);
      item_view_for_search->set_owned_by_client();
      shortcut_views_for_search_by_category_[category].emplace_back(
          std::move(item_view_for_search));
    }
  }

  // Init views of TabbedPane and KeyboardShortcutItemListView.
  tabbed_pane_ =
      new views::TabbedPane(views::TabbedPane::Orientation::kVertical);
  for (auto category : GetShortcutCategories()) {
    auto iter = shortcut_views_by_category_.find(category);
    // TODO(wutao): add DCKECK(iter != shortcut_views_by_category_.end()).
    if (iter != shortcut_views_by_category_.end()) {
      KeyboardShortcutItemListView* item_list_view =
          new KeyboardShortcutItemListView();
      for (auto* item_view : iter->second)
        item_list_view->AddItemView(item_view);
      tabbed_pane_->AddTab(GetStringForCategory(category), item_list_view);
    }
  }
  AddChildView(tabbed_pane_);
}

bool KeyboardShortcutView::CanMaximize() const {
  return false;
}

bool KeyboardShortcutView::CanMinimize() const {
  return true;
}

bool KeyboardShortcutView::CanResize() const {
  return false;
}

views::ClientView* KeyboardShortcutView::CreateClientView(
    views::Widget* widget) {
  return new views::ClientView(widget, this);
}

void KeyboardShortcutView::Layout() {
  gfx::Size search_box_ps = search_box_view_->GetPreferredSize();
  gfx::Rect bounds(search_box_ps);
  bounds.set_x((width() - search_box_ps.width()) / 2);
  bounds.set_y(kSearchBoxTopPadding);
  search_box_view_->SetBoundsRect(bounds);

  views::View* content_view;
  if (tabbed_pane_->visible())
    content_view = tabbed_pane_;
  else
    content_view = search_results_container_;
  const int used_height =
      search_box_ps.height() + kSearchBoxTopPadding + kSearchBoxBottomPadding;
  content_view->SetBounds(0, used_height, width(), height() - used_height);
}

void KeyboardShortcutView::BackButtonPressed() {
  search_box_view_->SetSearchBoxActive(false);
  search_box_view_->ClearSearch();
}

void KeyboardShortcutView::QueryChanged(search_box::SearchBoxViewBase* sender) {
  search_results_container_->RemoveAllChildViews(true);
  if (sender->IsSearchBoxTrimmedQueryEmpty()) {
    search_results_container_->AddChildView(search_start_view_.get());
    return;
  }

  KeyboardShortcutItemListView* item_list_view =
      new KeyboardShortcutItemListView();

  const base::string16& new_contents = sender->search_box()->text();
  base::i18n::FixedPatternStringSearchIgnoringCaseAndAccents finder(
      new_contents);

  bool found_result = false;
  for (auto category : GetShortcutCategories()) {
    auto iter = shortcut_views_for_search_by_category_.find(category);
    if (iter != shortcut_views_for_search_by_category_.end()) {
      bool first_result_in_category = true;
      for (const auto& item_view : iter->second) {
        base::string16 description_text =
            item_view->description_label_view()->text();
        base::string16 shortcut_text = item_view->shortcut_label_view()->text();
        if (finder.Search(description_text, nullptr, nullptr) ||
            finder.Search(shortcut_text, nullptr, nullptr)) {
          // TODO(wutao): highlight matched search text.
          if (first_result_in_category) {
            item_list_view->AddCategoryLabel(GetStringForCategory(category));
            first_result_in_category = false;
          }
          item_list_view->AddItemView(item_view.get());
          found_result = true;
        }
      }
    }
  }

  if (!found_result)
    search_results_container_->AddChildView(search_no_result_view_.get());
  else
    search_results_container_->AddChildView(item_list_view);
  Layout();
  SchedulePaint();
}

void KeyboardShortcutView::ActiveChanged(
    search_box::SearchBoxViewBase* sender) {
  if (sender->is_search_box_active()) {
    sender->ShowBackOrGoogleIcon(true);
    search_results_container_->SetVisible(true);
    tabbed_pane_->SetVisible(false);
  } else {
    sender->ShowBackOrGoogleIcon(false);
    search_results_container_->RemoveAllChildViews(true);
    search_results_container_->SetVisible(false);
    tabbed_pane_->SetVisible(true);
  }
  Layout();
  SchedulePaint();
}

KeyboardShortcutView* KeyboardShortcutView::GetInstanceForTests() {
  return g_ksv_view;
}

int KeyboardShortcutView::GetCategoryNumberForTests() const {
  return shortcut_views_by_category_.size();
}

int KeyboardShortcutView::GetTabCountForTests() const {
  return tabbed_pane_->GetTabCount();
}

}  // namespace keyboard_shortcut_viewer
