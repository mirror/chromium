// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#if defined(USE_AURA)

#include <stddef.h>

#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/language/chrome_language_detection_client.h"
#include "chrome/browser/translate/translate_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/translate/translate_bubble_view.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/test_switches.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/language/core/browser/url_language_histogram.h"
#include "components/translate/core/browser/translate_manager.h"
#include "components/translate/core/browser/translate_script.h"
#include "components/translate/core/browser/translate_ui_delegate.h"
#include "components/translate/core/common/translate_switches.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/browser_test_utils.h"
#include "net/http/http_status_code.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace {

const char kEnglishTestPath[] = "english_page.html";
const char kFrenchTestPath[] = "french_page.html";

using LanguageInfo = language::UrlLanguageHistogram::LanguageInfo;

};  // namespace

// Basic translate browser test with an embedded non-secure test server.
class TranslateLanguageBaseBrowserTest : public InProcessBrowserTest {
 public:
  TranslateLanguageBaseBrowserTest() {}

  void SetUp() override {
    scoped_feature_list_ = base::MakeUnique<base::test::ScopedFeatureList>();
    scoped_feature_list_->InitAndEnableFeature(
        kDecoupleTranslateLanguageFeature);
    set_open_about_blank_on_browser_launch(true);
    translate::TranslateManager::SetIgnoreMissingKeyForTesting(true);
    InProcessBrowserTest::SetUp();
  }

  void TearDown() override { InProcessBrowserTest::TearDown(); }

 private:
  std::unique_ptr<base::test::ScopedFeatureList> scoped_feature_list_;
  DISALLOW_COPY_AND_ASSIGN(TranslateLanguageBaseBrowserTest);
};

// Translate browser test for loading a page and checking infobar UI.
class TranslateLanguageBrowserTest : public TranslateLanguageBaseBrowserTest {
 public:
  TranslateLanguageBrowserTest()
      : browser_(nullptr), translate_model_(nullptr) {}

 protected:
  void SwitchToIncognitoMode() { browser_ = CreateIncognitoBrowser(); }

  void CheckForTranslateUI(const std::string& path, bool expect_translate) {
    if (!browser_) {
      browser_ = browser();
    }
    content::WebContents* current_web_contents =
        browser_->tab_strip_model()->GetActiveWebContents();
    content::Source<content::WebContents> source(current_web_contents);
    ui_test_utils::WindowedNotificationObserverWithDetails<
        translate::LanguageDetectionDetails>
        language_detected_signal(chrome::NOTIFICATION_TAB_LANGUAGE_DETERMINED,
                                 source);

    GURL url = ui_test_utils::GetTestUrl(
        base::FilePath(), base::FilePath(FILE_PATH_LITERAL(path)));
    ui_test_utils::NavigateToURL(browser_, url);
    language_detected_signal.Wait();
    TranslateBubbleView* bubble = TranslateBubbleView::GetCurrentBubble();
    if (expect_translate) {
      EXPECT_TRUE(bubble);
      translate_model_ = bubble->model();
    } else {
      EXPECT_FALSE(bubble);
    }
  }

  language::UrlLanguageHistogram* GetUrlLanguageHistogram() {
    content::WebContents* web_contents =
        browser_->tab_strip_model()->GetActiveWebContents();
    EXPECT_TRUE(web_contents);
    auto* lang_client =
        ChromeLanguageDetectionClient::FromWebContents(web_contents);
    EXPECT_TRUE(lang_client);
    return lang_client->GetUrlLanguageHistogram();
  }

  void Translate() {
    EXPECT_TRUE(translate_model_);
    translate_model_->Translate();
  }

  void RevertTranslation() {
    EXPECT_TRUE(translate_model_);
    translate_model_->RevertTranslation();
  }

 private:
  Browser* browser_;
  TranslateBubbleModel* translate_model_;
};

IN_PROC_BROWSER_TEST_F(TranslateLanguageBrowserTest, LanguageModelLogSucceed) {
  for (int i = 0; i < 10; ++i) {
    ASSERT_NO_FATAL_FAILURE(CheckForTranslateUI(kEnglishTestPath, false));
    ASSERT_NO_FATAL_FAILURE(CheckForTranslateUI(kFrenchTestPath, true));
  }
  // Intentionally visit the french page one more time.
  ASSERT_NO_FATAL_FAILURE(CheckForTranslateUI(kFrenchTestPath, true));

  // We should expect fr and en. fr should be 11 / (11 + 10) = 0.5238.
  language::UrlLanguageHistogram* histograms = GetUrlLanguageHistogram();
  EXPECT_TRUE(histograms);
  const std::vector<LanguageInfo>& langs = histograms->GetTopLanguages();
  EXPECT_EQ(2ul, langs.size());
  EXPECT_EQ("fr", langs[0].language_code);
  EXPECT_EQ("en", langs[1].language_code);
  EXPECT_NEAR(0.5238f, langs[0].frequency, 0.001f);
}

IN_PROC_BROWSER_TEST_F(TranslateLanguageBrowserTest, TranslateNRevert) {
  ASSERT_NO_FATAL_FAILURE(CheckForTranslateUI(kFrenchTestPath, true));
  ASSERT_NO_FATAL_FAILURE(Translate());
  ASSERT_NO_FATAL_FAILURE(RevertTranslation());
}

IN_PROC_BROWSER_TEST_F(TranslateLanguageBrowserTest, DontLogInIncognito) {
  SwitchToIncognitoMode();
  for (int i = 0; i < 10; ++i) {
    ASSERT_NO_FATAL_FAILURE(CheckForTranslateUI(kEnglishTestPath, false));
    ASSERT_NO_FATAL_FAILURE(CheckForTranslateUI(kFrenchTestPath, true));
  }
  // We should expect no url language histograms.
  language::UrlLanguageHistogram* histograms = GetUrlLanguageHistogram();
  EXPECT_FALSE(histograms);
}

#endif  // defined(USE_AURA)
