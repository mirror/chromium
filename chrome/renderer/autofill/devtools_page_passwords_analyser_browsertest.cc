// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/renderer/devtools_page_passwords_analyser.h"

#include "chrome/test/base/chrome_render_view_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebElementCollection.h"
#include "third_party/WebKit/public/web/WebFormElement.h"

#include <iostream>

namespace autofill {

namespace {

class TestLogger : public PasswordManagerLogger {
 public:
  void Send(const std::string& message,
            const ConsoleLevel level,
            const blink::WebNode& node) override {
    Send(message, level, std::vector<blink::WebNode>{node});
  }

  void Send(const std::string& message,
            const ConsoleLevel level,
            const std::vector<blink::WebNode>& nodes) override {
    ASSERT_EQ(1u, expected_.erase(Entry{message, level, nodes}));
  }

  void Flush() override {
    // If there are any remaining messages left in |expected_|, then not all
    // warnings have been generated correctly.
    ASSERT_TRUE(expected_.empty());
  }

  // The order in which we expect messages is ignored. This avoids issues caused
  // by changing implementations: the ordering is not important.
  void Expect(const std::string& message,
              const ConsoleLevel level,
              const std::vector<blink::WebNode>& nodes) {
    expected_.insert(Entry{message, level, nodes});
  }

 private:
  struct Entry {
    const std::string message;
    const ConsoleLevel level;
    const std::vector<blink::WebNode> nodes;
  };

  friend bool operator<(const Entry& lhs, const Entry& rhs);

  std::set<const Entry> expected_;
};

inline bool operator<(const TestLogger::Entry& lhs,
                      const TestLogger::Entry& rhs) {
  return std::tie(lhs.message, lhs.level, lhs.nodes) <
         std::tie(rhs.message, rhs.level, rhs.nodes);
}

const char kPasswordFieldNotInForm[] =
    "<input type='password' autocomplete='new-password'>";

const char kPasswordFormWithoutUsernameField[] =
    "<form>"
    "   <input type='password' autocomplete='new-password'>"
    "</form>";

const char kElementsWithDuplicateIds[] =
    "<input id='duplicate'>"
    "<input id='duplicate'>";

const char kPasswordFormTooComplex[] =
    "<form>"
    "   <input type='text' autocomplete='username'>"
    "   <input type='password' autocomplete='current-password'>"
    "   <input type='text' autocomplete='username'>"
    "   <input type='password' autocomplete='current-password'>"
    "   <input type='password' autocomplete='new-password'>"
    "   <input type='password' autocomplete='new-password'>"
    "</form>";

const char kInferredAutocompleteAttributes[] =
    // Login form.
    "<form>"
    "   <input type='text'>"
    "   <input type='password'>"
    "</form>"
    // Registration form.
    "<form>"
    "   <input type='text'>"
    "   <input type='password'>"
    "   <input type='password'>"
    "</form>"
    // Change password form.
    "<form>"
    "   <input type='text'>"
    "   <input type='password'>"
    "   <input type='password'>"
    "   <input type='password'>"
    "</form>";

const std::string AutocompleteSuggestionString(const std::string& suggestion) {
  return "Input elements should have autocomplete "
         "attributes (suggested: \"" +
         suggestion + "\"):";
}

}  // namespace

class DevToolsPagePasswordsAnalyserTest : public ChromeRenderViewTest {
 public:
  DevToolsPagePasswordsAnalyserTest() {}

  void TearDown() override {
    devtools_page_passwords_analyser.Reset();
    ChromeRenderViewTest::TearDown();
  }

  void LoadTestCase(const char* html) {
    elements_.clear();
    LoadHTML(html);
    blink::WebLocalFrame* frame = GetMainFrame();
    blink::WebElementCollection collection = frame->GetDocument().All();
    for (blink::WebElement element = collection.FirstItem(); !element.IsNull();
         element = collection.NextItem()) {
      elements_.push_back(element);
    }
    // Remove the <html>, <head> and <body> elements.
    elements_.erase(elements_.begin(), elements_.begin() + 3);
  }

  void Expect(const std::string& message,
              const ConsoleLevel level,
              const std::vector<size_t>& element_indices) {
    std::vector<blink::WebNode> nodes;
    for (size_t index : element_indices)
      nodes.push_back(elements_[index]);
    test_logger.Expect(message, level, nodes);
  }

  void RunTestCase() {
    devtools_page_passwords_analyser.AnalyseDocumentDOM(GetMainFrame(),
                                                        &test_logger);
  }

  DevToolsPagePasswordsAnalyser devtools_page_passwords_analyser;
  TestLogger test_logger;

 private:
  DISALLOW_COPY_AND_ASSIGN(DevToolsPagePasswordsAnalyserTest);

  std::vector<blink::WebElement> elements_;
};

TEST_F(DevToolsPagePasswordsAnalyserTest, PasswordFieldNotInForm) {
  LoadTestCase(kPasswordFieldNotInForm);

  Expect("Password field is not contained in a form:",
         PasswordManagerLogger::kVerbose, {0});

  RunTestCase();
}

TEST_F(DevToolsPagePasswordsAnalyserTest, PasswordFormWithoutUsernameField) {
  LoadTestCase(kPasswordFormWithoutUsernameField);

  Expect(
      "Password forms should have (optionally hidden) "
      "username fields for accessibility:",
      PasswordManagerLogger::kVerbose, {0});

  RunTestCase();
}

TEST_F(DevToolsPagePasswordsAnalyserTest, ElementsWithDuplicateIds) {
  LoadTestCase(kElementsWithDuplicateIds);

  Expect("Found 2 elements with non-unique id #duplicate:",
         PasswordManagerLogger::kError, {0, 1});

  RunTestCase();
}

TEST_F(DevToolsPagePasswordsAnalyserTest, PasswordFormTooComplex) {
  LoadTestCase(kPasswordFormTooComplex);

  Expect(
      "Multiple forms should be contained in their own "
      "form elements; break up complex forms into ones that represent a "
      "single action:",
      PasswordManagerLogger::kVerbose, {0});

  RunTestCase();
}

TEST_F(DevToolsPagePasswordsAnalyserTest, InferredAutocompleteAttributes) {
  LoadTestCase(kInferredAutocompleteAttributes);
  size_t element_index = 0;

  // Login form.
  element_index++;
  Expect(AutocompleteSuggestionString("username"),
         PasswordManagerLogger::kVerbose, {element_index++});
  Expect(AutocompleteSuggestionString("current-password"),
         PasswordManagerLogger::kVerbose, {element_index++});

  // Registration form.
  element_index++;
  Expect(AutocompleteSuggestionString("username"),
         PasswordManagerLogger::kVerbose, {element_index++});
  Expect(AutocompleteSuggestionString("new-password"),
         PasswordManagerLogger::kVerbose, {element_index++});
  Expect(AutocompleteSuggestionString("new-password"),
         PasswordManagerLogger::kVerbose, {element_index++});

  // Change password form.
  element_index++;
  Expect(AutocompleteSuggestionString("username"),
         PasswordManagerLogger::kVerbose, {element_index++});
  Expect(AutocompleteSuggestionString("current-password"),
         PasswordManagerLogger::kVerbose, {element_index++});
  Expect(AutocompleteSuggestionString("new-password"),
         PasswordManagerLogger::kVerbose, {element_index++});
  Expect(AutocompleteSuggestionString("new-password"),
         PasswordManagerLogger::kVerbose, {element_index++});

  RunTestCase();
}

}  // namespace autofill
