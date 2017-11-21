// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/autocomplete/chrome_autocomplete_scheme_classifier.h"
#include "chrome/browser/extensions/api/omnibox/omnibox_api_testbase.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "chrome/test/base/search_test_utils.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/browser/process_manager.h"
#include "extensions/test/extension_test_message_listener.h"
#include "extensions/test/result_catcher.h"
#include "third_party/metrics_proto/omnibox_event.pb.h"

namespace {

void InputKeys(Browser* browser, const std::vector<ui::KeyboardCode>& keys) {
  for (auto key : keys) {
    // Note that sending key presses can be flaky at times.
    ASSERT_TRUE(ui_test_utils::SendKeyPressSync(browser, key, false, false,
                                                false, false));
  }
}

}  // namespace

class KeywordEnteredTest : public OmniboxApiTest {
 public:
  void SetupOmnibox() {
    // The results depend on the TemplateURLService being loaded. Make sure it
    // is loaded so that the autocomplete results are consistent.
    search_test_utils::WaitForTemplateURLServiceToLoad(
        TemplateURLServiceFactory::GetForProfile(browser()->profile()));

    chrome::FocusLocationBar(browser());
    ASSERT_TRUE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));
  }

  struct ExpectedEntry {
    std::string keyword;
    AutocompleteProvider::Type type;
  };

  void VerifyResult(const std::vector<ExpectedEntry>& entries,
                    AutocompleteController* autocomplete_controller) {
    int i = 0;
    const AutocompleteResult& result = autocomplete_controller->result();
    EXPECT_EQ(entries.size(), result.size());
    for (auto& entry : entries) {
      EXPECT_EQ(base::ASCIIToUTF16(entry.keyword),
                result.match_at(i).fill_into_edit);
      EXPECT_EQ(entry.type, result.match_at(i).provider->type());
      ++i;
    }
  }
};

// Tests that the autocomplete popup doesn't reopen after accepting input for
// a given query.
// http://crbug.com/88552
IN_PROC_BROWSER_TEST_F(OmniboxApiTest, PopupStaysClosed) {
  ASSERT_TRUE(RunExtensionTest("omnibox/basic")) << message_;

  // The results depend on the TemplateURLService being loaded. Make sure it is
  // loaded so that the autocomplete results are consistent.
  Profile* profile = browser()->profile();
  search_test_utils::WaitForTemplateURLServiceToLoad(
      TemplateURLServiceFactory::GetForProfile(profile));

  LocationBar* location_bar = GetLocationBar(browser());
  OmniboxView* omnibox_view = location_bar->GetOmniboxView();
  AutocompleteController* autocomplete_controller =
      GetAutocompleteController(browser());
  OmniboxPopupModel* popup_model = omnibox_view->model()->popup_model();

  // Input a keyword query and wait for suggestions from the extension.
  omnibox_view->OnBeforePossibleChange();
  omnibox_view->SetUserText(base::ASCIIToUTF16("kw comman"));
  omnibox_view->OnAfterPossibleChange(true);
  WaitForAutocompleteDone(autocomplete_controller);
  EXPECT_TRUE(autocomplete_controller->done());
  EXPECT_TRUE(popup_model->IsOpen());

  // Quickly type another query and accept it before getting suggestions back
  // for the query. The popup will close after accepting input - ensure that it
  // does not reopen when the extension returns its suggestions.
  extensions::ResultCatcher catcher;

  // TODO: Rather than send this second request by talking to the controller
  // directly, figure out how to send it via the proper calls to
  // location_bar or location_bar->().
  AutocompleteInput input(base::ASCIIToUTF16("kw command"),
                          metrics::OmniboxEventProto::NTP,
                          ChromeAutocompleteSchemeClassifier(profile));
  autocomplete_controller->Start(input);
  location_bar->AcceptInput();
  WaitForAutocompleteDone(autocomplete_controller);
  EXPECT_TRUE(autocomplete_controller->done());
  // This checks that the keyword provider (via javascript)
  // gets told to navigate to the string "command".
  EXPECT_TRUE(catcher.GetNextResult()) << catcher.message();
  EXPECT_FALSE(popup_model->IsOpen());
}

// Tests deleting a deletable omnibox extension suggestion result.
IN_PROC_BROWSER_TEST_F(OmniboxApiTest, DeleteOmniboxSuggestionResult) {
  ASSERT_TRUE(RunExtensionTest("omnibox/delete_match")) << message_;

  // The results depend on the TemplateURLService being loaded. Make sure it is
  // loaded so that the autocomplete results are consistent.
  Profile* profile = browser()->profile();
  search_test_utils::WaitForTemplateURLServiceToLoad(
      TemplateURLServiceFactory::GetForProfile(profile));

  AutocompleteController* autocomplete_controller =
      GetAutocompleteController(browser());

  chrome::FocusLocationBar(browser());
  ASSERT_TRUE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));

  // Input a keyword query and wait for suggestions from the extension.
  InputKeys(browser(), {ui::VKEY_K, ui::VKEY_W, ui::VKEY_SPACE, ui::VKEY_D});

  WaitForAutocompleteDone(autocomplete_controller);
  EXPECT_TRUE(autocomplete_controller->done());

  // Peek into the controller to see if it has the results we expect.
  const AutocompleteResult& result = autocomplete_controller->result();
  ASSERT_EQ(4U, result.size()) << AutocompleteResultAsString(result);

  EXPECT_EQ(base::ASCIIToUTF16("kw d"), result.match_at(0).fill_into_edit);
  EXPECT_EQ(AutocompleteProvider::TYPE_KEYWORD,
            result.match_at(0).provider->type());
  EXPECT_FALSE(result.match_at(0).deletable);

  EXPECT_EQ(base::ASCIIToUTF16("kw n1"), result.match_at(1).fill_into_edit);
  EXPECT_EQ(AutocompleteProvider::TYPE_KEYWORD,
            result.match_at(1).provider->type());
  // Verify that the first omnibox extension suggestion is deletable.
  EXPECT_TRUE(result.match_at(1).deletable);

  EXPECT_EQ(base::ASCIIToUTF16("kw n2"), result.match_at(2).fill_into_edit);
  EXPECT_EQ(AutocompleteProvider::TYPE_KEYWORD,
            result.match_at(2).provider->type());
  // Verify that the second omnibox extension suggestion is not deletable.
  EXPECT_FALSE(result.match_at(2).deletable);

  EXPECT_EQ(base::ASCIIToUTF16("kw d"), result.match_at(3).fill_into_edit);
  EXPECT_EQ(AutocompleteMatchType::SEARCH_WHAT_YOU_TYPED,
            result.match_at(3).type);
  EXPECT_FALSE(result.match_at(3).deletable);

// This test portion is excluded from Mac because the Mac key combination
// FN+SHIFT+DEL used to delete an omnibox suggestion cannot be reproduced.
// This is because the FN key is not supported in interactive_test_util.h.
#if !defined(OS_MACOSX)
  ExtensionTestMessageListener delete_suggestion_listener(
      "onDeleteSuggestion: des1", false);

  // Skip the first suggestion result.
  EXPECT_TRUE(ui_test_utils::SendKeyPressSync(browser(), ui::VKEY_DOWN, false,
                                              false, false, false));
  // Delete the second suggestion result. On Linux, this is done via SHIFT+DEL.
  EXPECT_TRUE(ui_test_utils::SendKeyPressSync(browser(), ui::VKEY_DELETE, false,
                                              true, false, false));
  // Verify that the onDeleteSuggestion event was fired.
  ASSERT_TRUE(delete_suggestion_listener.WaitUntilSatisfied());

  // Verify that the first suggestion result was deleted. There should be one
  // less suggestion result, 3 now instead of 4.
  ASSERT_EQ(3U, result.size());
  EXPECT_EQ(base::ASCIIToUTF16("kw d"), result.match_at(0).fill_into_edit);
  EXPECT_EQ(base::ASCIIToUTF16("kw n2"), result.match_at(1).fill_into_edit);
  EXPECT_EQ(base::ASCIIToUTF16("kw d"), result.match_at(2).fill_into_edit);
#endif
}

// If an extension implements the onKeywordEntered() event to display suggest
// results, this test verifies that when a user inputs and accepts the
// extension's keyword, the suggest results specified in onKeywordEntered()
// are displayed.
IN_PROC_BROWSER_TEST_F(KeywordEnteredTest, Basic) {
  SetupOmnibox();
  const extensions::Extension* extension =
      LoadExtension(test_data_dir_.AppendASCII("omnibox/keyword_entered"));
  EXPECT_EQ("true",
            ExecuteScriptInBackgroundPage(
                extension->id(), "registerOnKeywordEnteredListener();"));
  EXPECT_EQ("true", ExecuteScriptInBackgroundPage(
                        extension->id(), "registerOnInputStartedListener();"));
  EXPECT_EQ("true", ExecuteScriptInBackgroundPage(
                        extension->id(), "registerOnInputChangedListener();"));

  AutocompleteController* autocomplete_controller =
      GetAutocompleteController(browser());

  ExtensionTestMessageListener keyword_entered_listener("onKeywordEntered",
                                                        false);

  // Input and trigger the extension keyword.
  InputKeys(browser(), {ui::VKEY_K, ui::VKEY_W, ui::VKEY_SPACE});
  WaitForAutocompleteDone(autocomplete_controller);
  EXPECT_TRUE(autocomplete_controller->done());

  // Verify that the onKeywordEntered event was fired.
  ASSERT_TRUE(keyword_entered_listener.WaitUntilSatisfied());

  // Peek into the controller to see if it has the results we expect. The
  // suggestion results for onKeywordEntered should be displayed.
  VerifyResult({{"kw", AutocompleteProvider::TYPE_SEARCH},
                {"kw entered 1", AutocompleteProvider::TYPE_KEYWORD},
                {"kw entered 2", AutocompleteProvider::TYPE_KEYWORD}},
               autocomplete_controller);

  ExtensionTestMessageListener input_started_listener("onInputStarted", false);

  // Input another key to trigger onInputChanged.
  InputKeys(browser(), {ui::VKEY_P});
  WaitForAutocompleteDone(autocomplete_controller);
  EXPECT_TRUE(autocomplete_controller->done());

  // Verify that the onInputedStarted event was fired.
  ASSERT_TRUE(input_started_listener.WaitUntilSatisfied());

  // Ensure that the matches for onKeywordEntered are not displayed, and instead
  // only the onInputChanged suggestion results are shown.
  VerifyResult({{"kw p", AutocompleteProvider::TYPE_KEYWORD},
                {"kw p 1", AutocompleteProvider::TYPE_KEYWORD},
                {"kw p", AutocompleteProvider::TYPE_SEARCH}},
               autocomplete_controller);
  // Note that the last suggestion result is always the option to search for
  // whatever text was inputted into the omnibox.
}

// Tests the input edge case where, for example, if the keyword is "ssh" and the
// user types "sshfoo" in the omnibox, keyword mode is entered when the users
// presses space within "sshfoo" to form "ssh foo". This test tests the case
// when both the onKeywordEntered and onInputChanged events are registered. In
// this case only the onInputChanged event listener should be fired, and its
// suggestion results should populate the omnibox.
IN_PROC_BROWSER_TEST_F(
    KeywordEnteredTest,
    InputEdgeCaseWithOnKeywordEnteredAndOnInputChangedEventsRegistered) {
  SetupOmnibox();
  const extensions::Extension* extension =
      LoadExtension(test_data_dir_.AppendASCII("omnibox/keyword_entered"));
  EXPECT_EQ("true",
            ExecuteScriptInBackgroundPage(
                extension->id(), "registerOnKeywordEnteredListener();"));
  EXPECT_EQ("true", ExecuteScriptInBackgroundPage(
                        extension->id(), "registerOnInputStartedListener();"));
  EXPECT_EQ("true", ExecuteScriptInBackgroundPage(
                        extension->id(), "registerOnInputChangedListener();"));

  AutocompleteController* autocomplete_controller =
      GetAutocompleteController(browser());

  // Input a string such that the keyword and additional input are coupled.
  // Because this extension's keyword is "kw", we input "kwp", where "p" is the
  // additional input.
  InputKeys(browser(), {ui::VKEY_K, ui::VKEY_W, ui::VKEY_P});
  WaitForAutocompleteDone(autocomplete_controller);
  EXPECT_TRUE(autocomplete_controller->done());

  // Peek into the controller to see if it has the results we expect.
  // onKeywordEntered suggestion results should not be displayed.
  VerifyResult({{"kwp", AutocompleteProvider::TYPE_SEARCH}},
               autocomplete_controller);

  ExtensionTestMessageListener input_started_listener("onInputStarted", false);

  // Edit the current omnibox input "kwp" such that it becomes "kw p". This
  // should trigger the onInputStarted and onInputChanged events in that order.
  InputKeys(browser(), {ui::VKEY_LEFT, ui::VKEY_SPACE});
  WaitForAutocompleteDone(autocomplete_controller);
  EXPECT_TRUE(autocomplete_controller->done());

  ASSERT_TRUE(input_started_listener.WaitUntilSatisfied());

  // Ensure that the matches for onKeywordEntered are not displayed, and instead
  // only the onInputChanged suggestion results are shown.
  VerifyResult({{"kw p", AutocompleteProvider::TYPE_KEYWORD},
                {"kw p 1", AutocompleteProvider::TYPE_KEYWORD},
                {"kw p", AutocompleteProvider::TYPE_SEARCH}},
               autocomplete_controller);
  // Note that the last suggestion result is always the option to search for
  // whatever text was inputted into the omnibox.
}

// Tests the input edge case where, for example, if the keyword is "ssh" and the
// user types "sshfoo" in the omnibox, keyword mode is entered when the users
// presses space within "sshfoo" to form "ssh foo".

// This test tests the case when onInputChanged event is registered and the
// onKeywordEntered event is not. In this case only onInputChanged suggestion
// results should populate the omnibox.
IN_PROC_BROWSER_TEST_F(KeywordEnteredTest,
                       InputEdgeCaseWithOnlyOnInputChangedEventRegistered) {
  SetupOmnibox();
  const extensions::Extension* extension =
      LoadExtension(test_data_dir_.AppendASCII("omnibox/keyword_entered"));
  EXPECT_EQ("true", ExecuteScriptInBackgroundPage(
                        extension->id(), "registerOnInputStartedListener();"));
  EXPECT_EQ("true", ExecuteScriptInBackgroundPage(
                        extension->id(), "registerOnInputChangedListener();"));

  AutocompleteController* autocomplete_controller =
      GetAutocompleteController(browser());

  // Input a string such that the keyword and input are joined, e.g. if the
  // keyword is "key", then such an input would be "keyfoo" instead of
  // "key foo".
  InputKeys(browser(), {ui::VKEY_K, ui::VKEY_W, ui::VKEY_P});
  WaitForAutocompleteDone(autocomplete_controller);
  EXPECT_TRUE(autocomplete_controller->done());

  // Peek into the controller to see if it has the results we expect.
  VerifyResult({{"kwp", AutocompleteProvider::TYPE_SEARCH}},
               autocomplete_controller);

  ExtensionTestMessageListener input_started_listener("onInputStarted", false);

  // Edit the current omnibox input "kwp" such that it becomes "kw p". This
  // should trigger onInputStarted and onInputChanged events in that order.
  InputKeys(browser(), {ui::VKEY_LEFT, ui::VKEY_SPACE});
  WaitForAutocompleteDone(autocomplete_controller);
  EXPECT_TRUE(autocomplete_controller->done());

  // Verify that the onInputedStarted event was fired.
  //
  // TODO(catmullings): Enforce ordering check such that onInputStarted precedes
  // onInputChanged.
  ASSERT_TRUE(input_started_listener.WaitUntilSatisfied());

  VerifyResult({{"kw p", AutocompleteProvider::TYPE_KEYWORD},
                {"kw p 1", AutocompleteProvider::TYPE_KEYWORD},
                {"kw p", AutocompleteProvider::TYPE_SEARCH}},
               autocomplete_controller);
  // Note that the last suggestion result is always the option to search for
  // whatever text was inputted into the omnibox.
}

// Tests the input edge case where, for example, if the keyword is "ssh" and the
// user types "sshfoo" in the omnibox, keyword mode is entered when the users
// presses space within "sshfoo" to form "ssh foo".

// This test tests the case when onKeywordEntered event is registered and the
// onInputChanged event is not. In this case, onKeywordEntered event listeners
// should not be fired.
IN_PROC_BROWSER_TEST_F(KeywordEnteredTest,
                       InputEdgeCaseWithOnlyOnKeyboardEnteredEventRegistered) {
  SetupOmnibox();
  const extensions::Extension* extension =
      LoadExtension(test_data_dir_.AppendASCII("omnibox/keyword_entered"));
  EXPECT_EQ("true",
            ExecuteScriptInBackgroundPage(
                extension->id(), "registerOnKeywordEnteredListener();"));
  EXPECT_EQ("true", ExecuteScriptInBackgroundPage(
                        extension->id(), "registerOnInputStartedListener();"));

  AutocompleteController* autocomplete_controller =
      GetAutocompleteController(browser());

  // Input a string such that the keyword and input are joined, e.g. if the
  // keyword is "key", then the such an input would be "keyfoo" instead of
  // "key foo".
  InputKeys(browser(), {ui::VKEY_K, ui::VKEY_W, ui::VKEY_P});
  WaitForAutocompleteDone(autocomplete_controller);
  EXPECT_TRUE(autocomplete_controller->done());

  // Peek into the controller to see if it has the results we expect. The
  // suggestion results for onKeywordEntered should be displayed.
  VerifyResult({{"kwp", AutocompleteProvider::TYPE_SEARCH}},
               autocomplete_controller);

  ExtensionTestMessageListener input_started_listener("onInputStarted", false);

  // Edit the current omnibox input "kwp" such that it becomes "kw p". This
  // should trigger onKeywordEntered and onInputStarted events in that order.
  InputKeys(browser(), {ui::VKEY_LEFT, ui::VKEY_SPACE});
  WaitForAutocompleteDone(autocomplete_controller);
  EXPECT_TRUE(autocomplete_controller->done());

  // Verify that the onInputedStarted event was fired.
  //
  // TODO(catmullings): Enforce ordering check such that onInputStarted precedes
  // onInputChanged.
  ASSERT_TRUE(input_started_listener.WaitUntilSatisfied());

  // No developer specified suggestions should be present.
  VerifyResult({{"kw p", AutocompleteProvider::TYPE_KEYWORD},
                {"kw p", AutocompleteProvider::TYPE_SEARCH}},
               autocomplete_controller);
  // Note that the last suggestion result is always the option to search for
  // whatever text was inputted into the omnibox.
}
