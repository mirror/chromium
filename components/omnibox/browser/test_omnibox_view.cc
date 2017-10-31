// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/test_omnibox_view.h"

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/omnibox_client.h"
#include "components/omnibox/browser/omnibox_edit_controller.h"
#include "components/omnibox/browser/omnibox_edit_model.h"
#include "components/omnibox/browser/omnibox_popup_model.h"
#include "components/omnibox/browser/omnibox_popup_view.h"
#include "components/omnibox/browser/omnibox_view.h"
#include "components/omnibox/browser/search_provider.h"
#include "components/omnibox/browser/test_omnibox_client.h"
#include "components/search_engines/search_terms_data.h"
#include "components/search_engines/template_url_service.h"
#include "components/search_engines/template_url_service_client.h"
#include "components/sessions/core/session_id.h"
#include "components/toolbar/test_toolbar_model.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/rect.h"

base::string16 TestOmniboxView::GetText() const {
  return text_;
}

void TestOmniboxView::SetUserText(const base::string16& text,
                                  bool update_popup) {
  text_ = text;
}
void TestOmniboxView::SetWindowTextAndCaretPos(const base::string16& text,
                                               size_t caret_pos,
                                               bool update_popup,
                                               bool notify_text_changed) {
  text_ = text;
}

bool TestOmniboxView::IsSelectAll() const {
  return false;
}

bool TestOmniboxView::DeleteAtEndPressed() {
  return false;
}

void TestOmniboxView::OnTemporaryTextMaybeChanged(
    const base::string16& display_text,
    bool save_original_selection,
    bool notify_text_changed) {
  text_ = display_text;
}

bool TestOmniboxView::OnInlineAutocompleteTextMaybeChanged(
    const base::string16& display_text,
    size_t user_text_length) {
  const bool text_changed = text_ != display_text;
  text_ = display_text;
  inline_autocomplete_text_ = display_text.substr(user_text_length);
  return text_changed;
}

void TestOmniboxView::OnInlineAutocompleteTextCleared() {
  inline_autocomplete_text_.clear();
}

bool TestOmniboxView::OnAfterPossibleChange(bool allow_keyword_ui_change) {
  return false;
}

gfx::NativeView TestOmniboxView::GetNativeView() const {
  return nullptr;
}

gfx::NativeView TestOmniboxView::GetRelativeWindowForPopup() const {
  return nullptr;
}

int TestOmniboxView::GetTextWidth() const {
  return 0;
}

int TestOmniboxView::GetWidth() const {
  return 0;
}

bool TestOmniboxView::IsImeComposing() const {
  return false;
}

int TestOmniboxView::GetOmniboxTextLength() const {
  return 0;
}
