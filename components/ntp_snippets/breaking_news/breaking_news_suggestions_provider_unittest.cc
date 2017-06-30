// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/breaking_news/breaking_news_suggestions_provider.h"

#include "base/files/scoped_temp_dir.h"
#include "base/json/json_reader.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/simple_test_clock.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/ntp_snippets/breaking_news/breaking_news_listener.h"
#include "components/ntp_snippets/breaking_news/breaking_news_suggestions_provider.h"
#include "components/ntp_snippets/mock_content_suggestions_provider_observer.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::Eq;
using testing::Property;
using testing::StrictMock;
using testing::SaveArg;
using testing::_;
namespace ntp_snippets {

namespace internal {

namespace {

class MockBreakingNewsListener : public BreakingNewsListener {
 public:
  MOCK_METHOD1(StartListening, void(OnNewContentCallback callback));
  MOCK_METHOD0(StopListening, void());
};

class BreakingNewsSuggestionsProviderTest : public testing::Test {
 public:
  BreakingNewsSuggestionsProviderTest() {
    EXPECT_TRUE(database_dir_.CreateUniqueTempDir());
  }

  void MakeSuggestionsProvider() {
    auto listener = base::MakeUnique<StrictMock<MockBreakingNewsListener>>();
    EXPECT_CALL(*listener, StartListening(_))
        .WillOnce(SaveArg<0>(&on_new_content_callback_));
    EXPECT_CALL(*listener, StopListening());

    scoped_refptr<base::SingleThreadTaskRunner> task_runner(
        base::ThreadTaskRunnerHandle::Get());
    // TODO(mamir): Use a mock DB instead of a real one. A DB interface needs to
    // be extracted for that.
    auto database = base::MakeUnique<RemoteSuggestionsDatabase>(
        database_dir_.GetPath(), task_runner);

    auto simple_test_clock = base::MakeUnique<base::SimpleTestClock>();

    provider_ = base::MakeUnique<BreakingNewsSuggestionsProvider>(
        &observer_, std::move(listener), std::move(simple_test_clock),
        std::move(database));

    // TODO(treib): Find a better way to wait for initialization to finish.
    base::RunLoop().RunUntilIdle();
  }

 protected:
  BreakingNewsListener::OnNewContentCallback on_new_content_callback_;
  base::MessageLoop message_loop_;
  std::unique_ptr<BreakingNewsSuggestionsProvider> provider_;
  StrictMock<MockContentSuggestionsProviderObserver> observer_;
  base::ScopedTempDir database_dir_;
};

TEST_F(BreakingNewsSuggestionsProviderTest, ReceivePushedBreakingNews) {
  MakeSuggestionsProvider();
  std::string json =
      "{\"categories\" : [{"
      "  \"id\": 8,"
      "  \"localizedTitle\": \"Breaking News\","
      "  \"suggestions\" : [{"
      "    \"ids\" : [\"http://localhost/foobar\"],"
      "    \"title\" : \"Foo Barred from Baz\","
      "    \"snippet\" : \"...\","
      "    \"fullPageUrl\" : \"http://localhost/fullUrl\","
      "    \"creationTime\" : \"2016-06-30T11:01:37.000Z\","
      "    \"expirationTime\" : \"2016-07-01T11:01:37.000Z\","
      "    \"attribution\" : \"Foo News\","
      "    \"imageUrl\" : \"http://localhost/foobar.jpg\","
      "    \"faviconUrl\" : \"http://localhost/favicon.ico\" "
      "  }]"
      "}]}";
  base::JSONReader json_reader;
  std::unique_ptr<base::Value> value = json_reader.ReadToValue(json);
  EXPECT_CALL(
      observer_,
      OnNewSuggestions(
          _, Eq(Category::FromKnownCategory(KnownCategories::BREAKING_NEWS)),
          ElementsAre(AllOf(
              Property(&ContentSuggestion::id,
                       ContentSuggestion::ID(Category::FromRemoteCategory(8),
                                             "http://localhost/foobar")),
              Property(&ContentSuggestion::title,
                       base::UTF8ToUTF16("Foo Barred from Baz")),
              Property(&ContentSuggestion::snippet_text,
                       base::UTF8ToUTF16("...")),
              Property(&ContentSuggestion::url,
                       GURL("http://localhost/fullUrl"))))));

  on_new_content_callback_.Run(std::move(value));
}

}  // namespace
}  // namespace internal
}  // namespace ntp_snippets
