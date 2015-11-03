// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/datausage/external_data_use_observer.h"

#include <string>
#include <vector>

#include "base/memory/scoped_ptr.h"
#include "base/run_loop.h"
#include "base/thread_task_runner_handle.h"
#include "components/data_usage/core/data_use.h"
#include "components/data_usage/core/data_use_aggregator.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/base/network_change_notifier.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace chrome {

namespace android {

class ExternalDataUseObserverTest : public testing::Test {
 public:
  void SetUp() override {
    thread_bundle_.reset(new content::TestBrowserThreadBundle(
        content::TestBrowserThreadBundle::REAL_IO_THREAD));
    io_task_runner_ = content::BrowserThread::GetMessageLoopProxyForThread(
        content::BrowserThread::IO);
    ui_task_runner_ = content::BrowserThread::GetMessageLoopProxyForThread(
        content::BrowserThread::UI);
    data_use_aggregator_.reset(new data_usage::DataUseAggregator());
    external_data_use_observer_.reset(new ExternalDataUseObserver(
        data_use_aggregator_.get(), io_task_runner_.get(),
        ui_task_runner_.get()));
  }

  ExternalDataUseObserver* external_data_use_observer() const {
    return external_data_use_observer_.get();
  }

 private:
  // Required for creating multiple threads for unit testing.
  scoped_ptr<content::TestBrowserThreadBundle> thread_bundle_;
  scoped_ptr<data_usage::DataUseAggregator> data_use_aggregator_;
  scoped_ptr<ExternalDataUseObserver> external_data_use_observer_;
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner_;
};

TEST_F(ExternalDataUseObserverTest, SingleRegex) {
  const struct {
    std::string url;
    std::string regex;
    bool expect_match;
  } tests[] = {
      {"http://www.google.com", "http://www.google.com/", true},
      {"http://www.Google.com", "http://www.google.com/", true},
      {"http://www.googleacom", "http://www.google.com/", true},
      {"http://www.googleaacom", "http://www.google.com/", false},
      {"http://www.google.com", "https://www.google.com/", false},
      {"http://www.google.com", "{http|https}://www[.]google[.]com/search.*",
       false},
      {"https://www.google.com/search=test",
       "https://www[.]google[.]com/search.*", true},
      {"https://www.googleacom/search=test",
       "https://www[.]google[.]com/search.*", false},
      {"https://www.google.com/Search=test",
       "https://www[.]google[.]com/search.*", true},
      {"www.google.com", "http://www.google.com", false},
      {"www.google.com:80", "http://www.google.com", false},
      {"http://www.google.com:80", "http://www.google.com", false},
      {"http://www.google.com:80/", "http://www.google.com/", true},
      {"", "http://www.google.com", false},
      {"", "", false},
      {"https://www.google.com", "http://www.google.com", false},
  };

  std::string label("test");
  for (size_t i = 0; i < arraysize(tests); ++i) {
    external_data_use_observer()->RegisterURLRegexes(
        std::vector<std::string>(1, std::string()),
        std::vector<std::string>(1, tests[i].regex),
        std::vector<std::string>(1, "label"));
    EXPECT_EQ(tests[i].expect_match,
              external_data_use_observer()->Matches(GURL(tests[i].url), &label))
        << i;

    // Verify label matches the expected label.
    std::string expected_label = "";
    if (tests[i].expect_match)
      expected_label = "label";

    EXPECT_EQ(expected_label, label);
  }
}

TEST_F(ExternalDataUseObserverTest, TwoRegex) {
  const struct {
    std::string url;
    std::string regex1;
    std::string regex2;
    bool expect_match;
  } tests[] = {
      {"http://www.google.com", "http://www.google.com/",
       "https://www.google.com/", true},
      {"http://www.googleacom", "http://www.google.com/",
       "http://www.google.com/", true},
      {"https://www.google.com", "http://www.google.com/",
       "https://www.google.com/", true},
      {"https://www.googleacom", "http://www.google.com/",
       "https://www.google.com/", true},
      {"http://www.google.com", "{http|https}://www[.]google[.]com/search.*",
       "", false},
      {"http://www.google.com/search=test",
       "http://www[.]google[.]com/search.*",
       "https://www[.]google[.]com/search.*", true},
      {"https://www.google.com/search=test",
       "http://www[.]google[.]com/search.*",
       "https://www[.]google[.]com/search.*", true},
      {"http://google.com/search=test", "http://www[.]google[.]com/search.*",
       "https://www[.]google[.]com/search.*", false},
      {"https://www.googleacom/search=test", "",
       "https://www[.]google[.]com/search.*", false},
      {"https://www.google.com/Search=test", "",
       "https://www[.]google[.]com/search.*", true},
      {"www.google.com", "http://www.google.com", "", false},
      {"www.google.com:80", "http://www.google.com", "", false},
      {"http://www.google.com:80", "http://www.google.com", "", false},
      {"", "http://www.google.com", "", false},
      {"https://www.google.com", "http://www.google.com", "", false},
  };

  std::string label;
  for (size_t i = 0; i < arraysize(tests); ++i) {
    std::vector<std::string> url_regexes;
    url_regexes.push_back(tests[i].regex1 + "|" + tests[i].regex2);
    external_data_use_observer()->RegisterURLRegexes(
        std::vector<std::string>(url_regexes.size(), std::string()),
        url_regexes, std::vector<std::string>(url_regexes.size(), "label"));
    EXPECT_EQ(tests[i].expect_match,
              external_data_use_observer()->Matches(GURL(tests[i].url), &label))
        << i;
  }
}

TEST_F(ExternalDataUseObserverTest, MultipleRegex) {
  std::vector<std::string> url_regexes;
  url_regexes.push_back(
      "https?://www[.]google[.]com/#q=.*|https?://www[.]google[.]com[.]ph/"
      "#q=.*|https?://www[.]google[.]com[.]ph/[?]gws_rd=ssl#q=.*");
  external_data_use_observer()->RegisterURLRegexes(
      std::vector<std::string>(url_regexes.size(), std::string()), url_regexes,
      std::vector<std::string>(url_regexes.size(), "label"));

  const struct {
    std::string url;
    bool expect_match;
  } tests[] = {
      {"", false},
      {"http://www.google.com", false},
      {"http://www.googleacom", false},
      {"https://www.google.com", false},
      {"https://www.googleacom", false},
      {"https://www.google.com", false},
      {"quic://www.google.com/q=test", false},
      {"http://www.google.com/q=test", false},
      {"http://www.google.com/.q=test", false},
      {"http://www.google.com/#q=test", true},
      {"https://www.google.com/#q=test", true},
      {"https://www.google.com.ph/#q=test+abc", true},
      {"https://www.google.com.ph/?gws_rd=ssl#q=test+abc", true},
      {"http://www.google.com.ph/#q=test", true},
      {"https://www.google.com.ph/#q=test", true},
      {"http://www.google.co.in/#q=test", false},
      {"http://google.com/#q=test", false},
      {"https://www.googleacom/#q=test", false},
      {"https://www.google.com/#Q=test", true},  // case in-sensitive
      {"www.google.com/#q=test", false},
      {"www.google.com:80/#q=test", false},
      {"http://www.google.com:80/#q=test", true},
      {"http://www.google.com:80/search?=test", false},
  };

  std::string label;
  for (size_t i = 0; i < arraysize(tests); ++i) {
    EXPECT_EQ(tests[i].expect_match,
              external_data_use_observer()->Matches(GURL(tests[i].url), &label))
        << i << " " << tests[i].url;
  }
}

TEST_F(ExternalDataUseObserverTest, ChangeRegex) {
  std::string label;
  // When no regex is specified, the URL match should fail.
  EXPECT_FALSE(external_data_use_observer()->Matches(GURL(""), &label));
  EXPECT_FALSE(external_data_use_observer()->Matches(
      GURL("http://www.google.com"), &label));

  std::vector<std::string> url_regexes;
  url_regexes.push_back("http://www[.]google[.]com/#q=.*");
  url_regexes.push_back("https://www[.]google[.]com/#q=.*");
  external_data_use_observer()->RegisterURLRegexes(
      std::vector<std::string>(url_regexes.size(), std::string()), url_regexes,
      std::vector<std::string>(url_regexes.size(), "label"));

  EXPECT_FALSE(external_data_use_observer()->Matches(GURL(""), &label));
  EXPECT_TRUE(external_data_use_observer()->Matches(
      GURL("http://www.google.com#q=abc"), &label));
  EXPECT_FALSE(external_data_use_observer()->Matches(
      GURL("http://www.google.co.in#q=abc"), &label));

  // Change the regular expressions to verify that the new regexes replace
  // the ones specified before.
  url_regexes.clear();
  url_regexes.push_back("http://www[.]google[.]co[.]in/#q=.*");
  url_regexes.push_back("https://www[.]google[.]co[.]in/#q=.*");
  external_data_use_observer()->RegisterURLRegexes(
      std::vector<std::string>(url_regexes.size(), std::string()), url_regexes,
      std::vector<std::string>(url_regexes.size(), "label"));
  EXPECT_FALSE(external_data_use_observer()->Matches(GURL(""), &label));
  EXPECT_FALSE(external_data_use_observer()->Matches(
      GURL("http://www.google.com#q=abc"), &label));
  EXPECT_TRUE(external_data_use_observer()->Matches(
      GURL("http://www.google.co.in#q=abc"), &label));
}

// Tests that at most one data use request is submitted, and if buffer size
// does not exceed the specified limit.
TEST_F(ExternalDataUseObserverTest, AtMostOneDataUseSubmitRequest) {
  const std::string label("label");

  std::vector<std::string> url_regexes;
  url_regexes.push_back(
      "http://www[.]google[.]com/#q=.*|https://www[.]google[.]com/#q=.*");

  external_data_use_observer()->FetchMatchingRulesCallbackOnIOThread(
      std::vector<std::string>(url_regexes.size(), std::string()), url_regexes,
      std::vector<std::string>(url_regexes.size(), label));
  EXPECT_EQ(0U, external_data_use_observer()->buffered_data_reports_.size());
  EXPECT_FALSE(external_data_use_observer()->submit_data_report_pending_);
  EXPECT_FALSE(external_data_use_observer()->matching_rules_fetch_pending_);

  std::vector<const data_usage::DataUse*> data_use_sequence;
  data_usage::DataUse data_use(
      GURL("http://www.google.com/#q=abc"), base::TimeTicks::Now(), GURL(), 0,
      net::NetworkChangeNotifier::CONNECTION_UNKNOWN, std::string(), 0, 0);
  data_use_sequence.push_back(&data_use);
  data_use_sequence.push_back(&data_use);
  external_data_use_observer()->OnDataUse(data_use_sequence);

  EXPECT_EQ(1U, external_data_use_observer()->buffered_data_reports_.size());
  EXPECT_TRUE(external_data_use_observer()->submit_data_report_pending_);

  const size_t max_buffer_size = ExternalDataUseObserver::kMaxBufferSize;

  data_use_sequence.clear();
  for (size_t i = 0; i < max_buffer_size; ++i)
    data_use_sequence.push_back(&data_use);

  external_data_use_observer()->OnDataUse(data_use_sequence);
  EXPECT_EQ(max_buffer_size,
            external_data_use_observer()->buffered_data_reports_.size());

  // Verify the label of the data use report.
  for (const auto& data_report :
       external_data_use_observer()->buffered_data_reports_) {
    EXPECT_EQ(label, data_report.label);
  }
}

// Tests the behavior when multiple matching rules are available.
TEST_F(ExternalDataUseObserverTest, MultipleMatchingRules) {
  std::vector<std::string> url_regexes;
  url_regexes.push_back(
      "http://www[.]foo[.]com/#q=.*|https://www[.]foo[.]com/#q=.*");
  url_regexes.push_back(
      "http://www[.]bar[.]com/#q=.*|https://www[.]bar[.]com/#q=.*");

  std::vector<std::string> labels;
  const std::string label_foo("label_foo");
  const std::string label_bar("label_bar");
  labels.push_back(label_foo);
  labels.push_back(label_bar);

  external_data_use_observer()->FetchMatchingRulesCallbackOnIOThread(
      std::vector<std::string>(url_regexes.size(), std::string()), url_regexes,
      labels);
  EXPECT_EQ(0U, external_data_use_observer()->buffered_data_reports_.size());
  EXPECT_FALSE(external_data_use_observer()->submit_data_report_pending_);
  EXPECT_FALSE(external_data_use_observer()->matching_rules_fetch_pending_);

  // Check |label_foo| matching rule.
  std::vector<const data_usage::DataUse*> data_use_sequence;
  data_usage::DataUse data_foo(
      GURL("http://www.foo.com/#q=abc"), base::TimeTicks::Now(), GURL(), 0,
      net::NetworkChangeNotifier::CONNECTION_UNKNOWN, std::string(), 0, 0);
  data_use_sequence.push_back(&data_foo);
  data_use_sequence.push_back(&data_foo);
  external_data_use_observer()->OnDataUse(data_use_sequence);

  EXPECT_EQ(1U, external_data_use_observer()->buffered_data_reports_.size());
  EXPECT_TRUE(external_data_use_observer()->submit_data_report_pending_);

  // Verify the label of the data use report.
  EXPECT_EQ(
      label_foo,
      external_data_use_observer()->buffered_data_reports_.begin()->label);

  // Clear the state.
  external_data_use_observer()->buffered_data_reports_.clear();
  data_use_sequence.clear();

  // Check |label_bar| matching rule.
  data_usage::DataUse data_bar(
      GURL("http://www.bar.com/#q=abc"), base::TimeTicks::Now(), GURL(), 0,
      net::NetworkChangeNotifier::CONNECTION_UNKNOWN, std::string(), 0, 0);
  data_use_sequence.push_back(&data_bar);
  external_data_use_observer()->OnDataUse(data_use_sequence);
  for (const auto& data_report :
       external_data_use_observer()->buffered_data_reports_) {
    EXPECT_EQ(label_bar, data_report.label);
  }
}

}  // namespace android

}  // namespace chrome
