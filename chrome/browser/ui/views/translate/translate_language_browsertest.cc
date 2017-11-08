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
#include "chrome/browser/language/url_language_histogram_factory.h"
#include "chrome/browser/translate/translate_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/translate/translate_bubble_model.h"
#include "chrome/browser/ui/translate/translate_bubble_test_utils.h"
#include "chrome/browser/ui/views/translate/translate_bubble_view.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/test_switches.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/language/core/browser/url_language_histogram.h"
#include "components/translate/core/browser/translate_manager.h"
#include "components/translate/core/browser/translate_ui_delegate.h"
#include "components/translate/core/common/language_detection_details.h"
#include "components/translate/core/common/translate_switches.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/browser_test_utils.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

namespace {

const base::FilePath::CharType kEnglishTestPath[] =
    FILE_PATH_LITERAL("english_page.html");
const base::FilePath::CharType kItalianPage[] =
    FILE_PATH_LITERAL("italian_page.html");
const base::FilePath::CharType kFrenchTestPath[] =
    FILE_PATH_LITERAL("french_page.html");

static const char kTestValidScript[] =
    "var google = {};"
    "google.translate = (function() {"
    "  return {"
    "    TranslateService: function() {"
    "      return {"
    "        isAvailable : function() {"
    "          return true;"
    "        },"
    "        restore : function() {"
    "          return;"
    "        },"
    "        getDetectedLanguage : function() {"
    "          return \"und\";"
    "        },"
    "        translatePage : function(originalLang, targetLang,"
    "                                 onTranslateProgress) {"
    "          onTranslateProgress(100, true, false);"
    "        }"
    "      };"
    "    }"
    "  };"
    "})();"
    "cr.googleTranslate.onTranslateElementLoad();";

using translate::test_utils::GetCurrentModel;

using LanguageInfo = language::UrlLanguageHistogram::LanguageInfo;

};  // namespace

class TranslateLanguageBrowserTest : public InProcessBrowserTest {
 public:
  TranslateLanguageBrowserTest() : browser_(nullptr) {}

  void SetUp() override {
    set_open_about_blank_on_browser_launch(true);
    translate::TranslateManager::SetIgnoreMissingKeyForTesting(true);
    InProcessBrowserTest::SetUp();
  }

  void TearDown() override { InProcessBrowserTest::TearDown(); }

 protected:
  void InitInIncognitoMode(const bool incognito) {
    browser_ = incognito ? CreateIncognitoBrowser() : browser();
  }

  void CheckForTranslateUI(const base::FilePath& path,
                           const bool expect_translate) {
    CHECK(browser_);

    content::WindowedNotificationObserver language_detected_signal(
        chrome::NOTIFICATION_TAB_LANGUAGE_DETERMINED,
        content::NotificationService::AllSources());

    const GURL url = ui_test_utils::GetTestUrl(base::FilePath(), path);
    ui_test_utils::NavigateToURL(browser_, url);
    language_detected_signal.Wait();
    TranslateBubbleView* const bubble = TranslateBubbleView::GetCurrentBubble();
    CHECK_NE(expect_translate, bubble == nullptr);
  }

  language::UrlLanguageHistogram* GetUrlLanguageHistogram() {
    const content::WebContents* const web_contents =
        browser_->tab_strip_model()->GetActiveWebContents();
    EXPECT_TRUE(web_contents);
    content::BrowserContext* const browser_context =
        web_contents->GetBrowserContext();
    EXPECT_TRUE(browser_context);
    return UrlLanguageHistogramFactory::GetForBrowserContext(browser_context);
  }

  void SetTargetLanguageByDisplayName(const base::string16& name) {
    translate::test_utils::SelectTargetLanguageByDisplayName(browser_, name);
  }

  void Translate(const bool first_translate) {
    content::WindowedNotificationObserver page_translated_signal(
        chrome::NOTIFICATION_PAGE_TRANSLATED,
        content::NotificationService::AllSources());

    EXPECT_EQ(TranslateBubbleModel::VIEW_STATE_BEFORE_TRANSLATE,
              GetCurrentModel(browser_)->GetViewState());

    translate::test_utils::PressTranslate(browser_);
    if (first_translate)
      SimulateURLFetch();

    page_translated_signal.Wait();
    EXPECT_EQ(TranslateBubbleModel::VIEW_STATE_AFTER_TRANSLATE,
              GetCurrentModel(browser_)->GetViewState());
  }

  void Revert() {
    translate::test_utils::PressRevert(browser_);
  }

  translate::LanguageState& GetLanguageState() {
    auto* const client = ChromeTranslateClient::FromWebContents(
        browser_->tab_strip_model()->GetActiveWebContents());
    return client->GetLanguageState();
  }

  std::unique_ptr<translate::TranslatePrefs> GetTranslatePrefs() {
    auto* const client = ChromeTranslateClient::FromWebContents(
        browser_->tab_strip_model()->GetActiveWebContents());
    return client->GetTranslatePrefs();
  }

 private:
  Browser* browser_;
  net::TestURLFetcherFactory url_fetcher_factory_;

  void SimulateURLFetch() {
    net::TestURLFetcher* const fetcher = url_fetcher_factory_.GetFetcherByID(0);
    ASSERT_TRUE(fetcher);

    fetcher->set_url(fetcher->GetOriginalURL());
    fetcher->set_status(net::URLRequestStatus::FromError(net::OK));
    fetcher->set_response_code(200);
    fetcher->SetResponseString(kTestValidScript);
    fetcher->delegate()->OnURLFetchComplete(fetcher);
  }

  DISALLOW_COPY_AND_ASSIGN(TranslateLanguageBrowserTest);
};

IN_PROC_BROWSER_TEST_F(TranslateLanguageBrowserTest, LanguageModelLogSucceed) {
  InitInIncognitoMode(false);

  for (int i = 0; i < 10; ++i) {
    ASSERT_NO_FATAL_FAILURE(
        CheckForTranslateUI(base::FilePath(kFrenchTestPath), true));
    EXPECT_EQ("fr", GetLanguageState().current_language());
    ASSERT_NO_FATAL_FAILURE(
        CheckForTranslateUI(base::FilePath(kEnglishTestPath), false));
    EXPECT_EQ("en", GetLanguageState().current_language());
  }
  // Intentionally visit the french page one more time.
  ASSERT_NO_FATAL_FAILURE(
      CheckForTranslateUI(base::FilePath(kFrenchTestPath), true));
  EXPECT_EQ("fr", GetLanguageState().current_language());

  // We should expect fr and en. fr should be 11 / (11 + 10) = 0.5238.
  const language::UrlLanguageHistogram* const histograms =
      GetUrlLanguageHistogram();
  ASSERT_TRUE(histograms);
  const std::vector<LanguageInfo>& langs = histograms->GetTopLanguages();
  EXPECT_EQ(2u, langs.size());
  EXPECT_EQ("fr", langs[0].language_code);
  EXPECT_EQ("en", langs[1].language_code);
  EXPECT_NEAR(11.0 / (11.0 + 10.0), langs[0].frequency, 0.001f);
  EXPECT_NEAR(10.0 / (11.0 + 10.0), langs[1].frequency, 0.001f);
}

IN_PROC_BROWSER_TEST_F(TranslateLanguageBrowserTest, DontLogInIncognito) {
  InitInIncognitoMode(true);

  for (int i = 0; i < 10; ++i) {
    ASSERT_NO_FATAL_FAILURE(
        CheckForTranslateUI(base::FilePath(kEnglishTestPath), false));
    EXPECT_EQ("en", GetLanguageState().current_language());
    ASSERT_NO_FATAL_FAILURE(
        CheckForTranslateUI(base::FilePath(kFrenchTestPath), true));
    EXPECT_EQ("fr", GetLanguageState().current_language());
  }
  // We should expect no url language histograms.
  const language::UrlLanguageHistogram* const histograms =
      GetUrlLanguageHistogram();
  EXPECT_FALSE(histograms);
}

IN_PROC_BROWSER_TEST_F(TranslateLanguageBrowserTest, TranslateAndRevert) {
  InitInIncognitoMode(false);

  // Visit the french page.
  ASSERT_NO_FATAL_FAILURE(
      CheckForTranslateUI(base::FilePath(kFrenchTestPath), true));
  EXPECT_EQ("fr", GetLanguageState().current_language());

  // Translate the page.
  ASSERT_NO_FATAL_FAILURE(Translate(true));
  EXPECT_EQ("en", GetLanguageState().current_language());

  // Revert the page.
  ASSERT_NO_FATAL_FAILURE(Revert());
  EXPECT_EQ("fr", GetLanguageState().current_language());
}

IN_PROC_BROWSER_TEST_F(TranslateLanguageBrowserTest, RecentTargetLanguage) {
  base::test::ScopedFeatureList enable_feature;
  enable_feature.InitAndEnableFeature(translate::kTranslateRecentTarget);

  InitInIncognitoMode(false);

  // Before browsing: set auto translate from French to Chinese.
  GetTranslatePrefs()->WhitelistLanguagePair("fr", "zh-CN");

  // Load an Italian page and translate to Spanish. After this, Spanish should
  // be our recent target language.
  ASSERT_NO_FATAL_FAILURE(
      CheckForTranslateUI(base::FilePath(kItalianPage), true));
  EXPECT_EQ("it", GetLanguageState().current_language());
  ASSERT_NO_FATAL_FAILURE(
      SetTargetLanguageByDisplayName(base::ASCIIToUTF16("Spanish")));
  ASSERT_NO_FATAL_FAILURE(Translate(true));
  EXPECT_EQ("es", GetLanguageState().current_language());

  // Load a French page. This should trigger an auto-translate to Chinese, but
  // not a recent target update.
  ASSERT_NO_FATAL_FAILURE(
      CheckForTranslateUI(base::FilePath(kFrenchTestPath), true));
  content::WindowedNotificationObserver page_translated_signal(
      chrome::NOTIFICATION_PAGE_TRANSLATED,
      content::NotificationService::AllSources());
  page_translated_signal.Wait();
  EXPECT_EQ("zh-CN", GetLanguageState().current_language());

  // Revert the translation. This should not clear recent target language (since
  // we didn't translate to that).
  ASSERT_NO_FATAL_FAILURE(Revert());
  EXPECT_EQ("fr", GetLanguageState().current_language());

  // Load an English page. This should offer to translate to Spanish, since that
  // is still our recent target language.
  ASSERT_NO_FATAL_FAILURE(
      CheckForTranslateUI(base::FilePath(kEnglishTestPath), true));
  EXPECT_EQ("en", GetLanguageState().current_language());
  ASSERT_NO_FATAL_FAILURE(Translate(false));
  EXPECT_EQ("es", GetLanguageState().current_language());

  // Revert the translation. This should clear the recent target language.
  ASSERT_NO_FATAL_FAILURE(Revert());
  EXPECT_EQ("en", GetLanguageState().current_language());

  // Load an Italian page again. We should be offered an English translation
  // (since we now have no recent target language).
  ASSERT_NO_FATAL_FAILURE(
      CheckForTranslateUI(base::FilePath(kItalianPage), true));
  EXPECT_EQ("it", GetLanguageState().current_language());
  ASSERT_NO_FATAL_FAILURE(Translate(false));
  EXPECT_EQ("en", GetLanguageState().current_language());
}

#endif  // defined(USE_AURA)
