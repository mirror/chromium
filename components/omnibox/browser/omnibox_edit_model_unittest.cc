// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/omnibox_edit_model.h"

#include <stddef.h>

#include <memory>

#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/omnibox_view.h"
#include "components/omnibox/browser/search_provider.h"
#include "components/omnibox/browser/test_omnibox_client.h"
#include "components/omnibox/browser/test_omnibox_edit_controller.h"
#include "components/omnibox/browser/test_omnibox_view.h"
#include "components/toolbar/test_toolbar_model.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class TestOmniboxEditModel : public OmniboxEditModel {
 public:
  TestOmniboxEditModel(OmniboxView* view, OmniboxEditController* controller)
      : OmniboxEditModel(view,
                         controller,
                         std::make_unique<TestOmniboxClient>()),
        popup_is_open_(false) {}

  bool PopupIsOpen() const override { return popup_is_open_; };

  AutocompleteMatch CurrentMatch(GURL*) const override {
    return current_match_;
  };

  void SetPopupIsOpen(bool open) { popup_is_open_ = open; }

  void SetCurrentMatch(const AutocompleteMatch& match) {
    current_match_ = match;
  }

 private:
  bool popup_is_open_;
  AutocompleteMatch current_match_;

  DISALLOW_COPY_AND_ASSIGN(TestOmniboxEditModel);
};

}  // namespace

class OmniboxEditModelTest : public testing::Test {
 public:
  void SetUp() override {
    controller_ = std::make_unique<TestOmniboxEditController>();
    view_ = std::make_unique<TestOmniboxView>(controller_.get());
    model_ =
        std::make_unique<TestOmniboxEditModel>(view_.get(), controller_.get());
  }

  const TestOmniboxView& view() { return *view_; }
  TestToolbarModel* toolbar_model() { return controller_->GetToolbarModel(); }
  TestOmniboxEditModel* model() { return model_.get(); }

 private:
  std::unique_ptr<TestOmniboxEditController> controller_;
  std::unique_ptr<TestOmniboxView> view_;
  std::unique_ptr<TestOmniboxEditModel> model_;
};

// Tests various permutations of AutocompleteModel::AdjustTextForCopy.
TEST_F(OmniboxEditModelTest, AdjustTextForCopy) {
  struct Data {
    const char* url_for_editing;
    const int sel_start;
    const bool is_all_selected;

    const char* match_destination_url;
    const bool is_match_selected_in_popup;

    const char* input;
    const char* expected_output;
    const bool write_url;
    const char* expected_url;
  } input[] = {
      // Test that http:// is inserted if all text is selected.
      {"a.de/b", 0, true, "", false, "a.de/b", "http://a.de/b", true,
       "http://a.de/b"},

      // Test that http:// is inserted if the host is selected.
      {"a.de/b", 0, false, "", false, "a.de/", "http://a.de/", true,
       "http://a.de/"},

      // Tests that http:// is inserted if the path is modified.
      {"a.de/b", 0, false, "", false, "a.de/c", "http://a.de/c", true,
       "http://a.de/c"},

      // Tests that http:// isn't inserted if the host is modified.
      {"a.de/b", 0, false, "", false, "a.com/b", "a.com/b", false, ""},

      // Tests that http:// isn't inserted if the start of the selection is 1.
      {"a.de/b", 1, false, "", false, "a.de/b", "a.de/b", false, ""},

      // Tests that http:// isn't inserted if a portion of the host is selected.
      {"a.de/", 0, false, "", false, "a.d", "a.d", false, ""},

      // Tests that http:// isn't inserted for an https url after the user nukes
      // https.
      {"https://a.com/", 0, false, "", false, "a.com/", "a.com/", false, ""},

      // Tests that http:// isn't inserted if the user adds to the host.
      {"a.de/", 0, false, "", false, "a.de.com/", "a.de.com/", false, ""},

      // Tests that we don't get double http if the user manually inserts http.
      {"a.de/", 0, false, "", false, "http://a.de/", "http://a.de/", true,
       "http://a.de/"},

      // Makes sure intranet urls get 'http://' prefixed to them.
      {"b/foo", 0, true, "", false, "b/foo", "http://b/foo", true,
       "http://b/foo"},

      // Verifies a search term 'foo' doesn't end up with http.
      {"www.google.com/search?", 0, false, "", false, "foo", "foo", false, ""},

      // Verifies that http:// is inserted for a match in a popup.
      {"a.com", 0, true, "http://b.com", true, "b.com/foo", "http://b.com/foo",
       true, "http://b.com/foo"},

      // Verifies that http:// isn't inserted if there is no valid match.
      {"a.com", 0, true, "", true, "b.com/foo", "b.com/foo", false, ""},
  };

  for (size_t i = 0; i < arraysize(input); ++i) {
    toolbar_model()->set_text(base::ASCIIToUTF16(input[i].url_for_editing));
    model()->ResetDisplayUrls();

    model()->SetInputInProgress(input[i].is_match_selected_in_popup);
    model()->SetPopupIsOpen(input[i].is_match_selected_in_popup);
    AutocompleteMatch match;
    match.type = AutocompleteMatchType::NAVSUGGEST;
    match.destination_url = GURL(input[i].match_destination_url);
    model()->SetCurrentMatch(match);

    base::string16 result = base::ASCIIToUTF16(input[i].input);
    GURL url;
    bool write_url;
    model()->AdjustTextForCopy(input[i].sel_start, input[i].is_all_selected,
                               &result, &url, &write_url);
    EXPECT_EQ(base::ASCIIToUTF16(input[i].expected_output), result)
        << "@: " << i;
    EXPECT_EQ(input[i].write_url, write_url) << " @" << i;
    if (write_url)
      EXPECT_EQ(input[i].expected_url, url.spec()) << " @" << i;
  }

  // AdjustTextForCopy does not currently account for HTTPS elision. This
  // assertion reminds us to update AdjustTextForCopy before enabling that
  // feature by default.
  ASSERT_EQ(0U, AutocompleteMatch::GetFormatTypes(false, false, false) &
                    url_formatter::kFormatUrlOmitHTTPS);
}

TEST_F(OmniboxEditModelTest, InlineAutocompleteText) {
  // Test if the model updates the inline autocomplete text in the view.
  EXPECT_EQ(base::string16(), view().inline_autocomplete_text());
  model()->SetUserText(base::ASCIIToUTF16("he"));
  model()->OnPopupDataChanged(base::ASCIIToUTF16("llo"), nullptr,
                              base::string16(), false);
  EXPECT_EQ(base::ASCIIToUTF16("hello"), view().GetText());
  EXPECT_EQ(base::ASCIIToUTF16("llo"), view().inline_autocomplete_text());

  base::string16 text_before = base::ASCIIToUTF16("he");
  base::string16 text_after = base::ASCIIToUTF16("hel");
  OmniboxView::StateChanges state_changes{
      &text_before, &text_after, 3, 3, false, true, false, false};
  model()->OnAfterPossibleChange(state_changes, true);
  EXPECT_EQ(base::string16(), view().inline_autocomplete_text());
  model()->OnPopupDataChanged(base::ASCIIToUTF16("lo"), nullptr,
                              base::string16(), false);
  EXPECT_EQ(base::ASCIIToUTF16("hello"), view().GetText());
  EXPECT_EQ(base::ASCIIToUTF16("lo"), view().inline_autocomplete_text());

  model()->Revert();
  EXPECT_EQ(base::string16(), view().GetText());
  EXPECT_EQ(base::string16(), view().inline_autocomplete_text());

  model()->SetUserText(base::ASCIIToUTF16("he"));
  model()->OnPopupDataChanged(base::ASCIIToUTF16("llo"), nullptr,
                              base::string16(), false);
  EXPECT_EQ(base::ASCIIToUTF16("hello"), view().GetText());
  EXPECT_EQ(base::ASCIIToUTF16("llo"), view().inline_autocomplete_text());

  model()->AcceptTemporaryTextAsUserText();
  EXPECT_EQ(base::ASCIIToUTF16("hello"), view().GetText());
  EXPECT_EQ(base::string16(), view().inline_autocomplete_text());
}

// This verifies the fix for a bug where calling OpenMatch() with a valid
// alternate nav URL would fail a DCHECK if the input began with "http://".
// The failure was due to erroneously trying to strip the scheme from the
// resulting fill_into_edit.  Alternate nav matches are never shown, so there's
// no need to ever try and strip this scheme.
TEST_F(OmniboxEditModelTest, AlternateNavHasHTTP) {
  const TestOmniboxClient* client =
      static_cast<TestOmniboxClient*>(model()->client());
  const AutocompleteMatch match(
      model()->autocomplete_controller()->search_provider(), 0, false,
      AutocompleteMatchType::SEARCH_WHAT_YOU_TYPED);
  const GURL alternate_nav_url("http://abcd/");

  model()->OnSetFocus(false);  // Avoids DCHECK in OpenMatch().
  model()->SetUserText(base::ASCIIToUTF16("http://abcd"));
  model()->OpenMatch(match, WindowOpenDisposition::CURRENT_TAB,
                     alternate_nav_url, base::string16(), 0);
  EXPECT_TRUE(AutocompleteInput::HasHTTPScheme(
      client->alternate_nav_match().fill_into_edit));

  model()->SetUserText(base::ASCIIToUTF16("abcd"));
  model()->OpenMatch(match, WindowOpenDisposition::CURRENT_TAB,
                     alternate_nav_url, base::string16(), 0);
  EXPECT_TRUE(AutocompleteInput::HasHTTPScheme(
      client->alternate_nav_match().fill_into_edit));
}
