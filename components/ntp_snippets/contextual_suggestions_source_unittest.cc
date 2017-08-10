// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/contextual_suggestions_source.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/test/mock_callback.h"
#include "components/image_fetcher/core/image_fetcher_impl.h"
#include "components/ntp_snippets/category_info.h"
#include "components/ntp_snippets/content_suggestion.h"
#include "components/ntp_snippets/remote/cached_image_fetcher.h"
#include "components/ntp_snippets/remote/contextual_suggestions_fetcher.h"
#include "components/ntp_snippets/remote/json_to_categories.h"
#include "components/ntp_snippets/remote/remote_suggestion.h"
#include "components/ntp_snippets/remote/remote_suggestion_builder.h"
#include "components/ntp_snippets/remote/remote_suggestions_database.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_unittest_util.h"

using testing::_;
using testing::AllOf;
using testing::ElementsAre;
using testing::Eq;
using testing::IsEmpty;
using testing::Mock;
using testing::Property;

namespace ntp_snippets {

namespace {

ACTION_P(SaveFirstIDFromContextualSuggestionsCallback, pointer) {
  *pointer = ::testing::get<2>(args)[0].id();
}

// Always fetches the result that was set by SetFakeResponse.
class FakeContextualSuggestionsFetcher : public ContextualSuggestionsFetcher {
 public:
  void FetchContextualSuggestions(
      const GURL& url,
      SuggestionsAvailableCallback callback) override {
    OptionalSuggestions suggestions;
    if (!fake_url_.empty()) {
      suggestions = RemoteSuggestion::PtrVector();
      suggestions->push_back(test::RemoteSuggestionBuilder()
                                 .AddId(fake_url_)
                                 .SetUrl(fake_url_)
                                 .SetAmpUrl(fake_url_)
                                 .SetImageUrl(fake_image_url_)
                                 .Build());
    }
    std::move(callback).Run(fake_status_, std::move(suggestions));
  }

  void SetFakeResponse(Status fake_status,
                       const std::string& fake_url,
                       const std::string& fake_image_url = "") {
    fake_image_url_ = fake_image_url;
    fake_status_ = fake_status;
    fake_url_ = fake_url;
  }

  const std::string& GetLastStatusForTesting() const override { return empty_; }
  const std::string& GetLastJsonForTesting() const override { return empty_; }
  const GURL& GetFetchUrlForTesting() const override { return empty_url_; }

 private:
  std::string empty_;
  GURL empty_url_;
  Status fake_status_ = Status::Success();
  std::string fake_image_url_;
  std::string fake_url_;
};

// Always fetches a fake image if the given URL is valid
class FakeCachedImageFetcher : public CachedImageFetcher {
 public:
  FakeCachedImageFetcher(PrefService* pref_service)
      : CachedImageFetcher(std::unique_ptr<image_fetcher::ImageFetcher>(),
                           pref_service,
                           nullptr){};

  void FetchSuggestionImage(const ContentSuggestion::ID&,
                            const GURL& image_url,
                            ImageFetchedCallback callback) override {
    gfx::Image image;
    if (image_url.is_valid()) {
      image = gfx::test::CreateImage();
    }
    std::move(callback).Run(image);
  }
};

// GMock does not support movable-only types (ContentSuggestion).
// Instead WrappedRun is used as callback and it redirects the call to a
// method without movable-only types, which is then mocked.
class MockFetchContextualSuggestionsCallback {
 public:
  void WrappedRun(Status status,
                  const GURL& url,
                  std::vector<ContentSuggestion> suggestions) {
    Run(status, url, suggestions);
  }

  ContextualSuggestionsSource::FetchContextualSuggestionsCallback
  ToOnceCallback() {
    return base::BindOnce(&MockFetchContextualSuggestionsCallback::WrappedRun,
                          base::Unretained(this));
  }

  MOCK_METHOD3(Run,
               void(Status status_code,
                    const GURL& url,
                    const std::vector<ContentSuggestion>& suggestions));
};

}  // namespace

class ContextualSuggestionsSourceTest : public testing::Test {
 public:
  ContextualSuggestionsSourceTest() {
    RequestThrottler::RegisterProfilePrefs(pref_service_.registry());
    std::unique_ptr<FakeContextualSuggestionsFetcher> fetcher =
        base::MakeUnique<FakeContextualSuggestionsFetcher>();
    fetcher_ = fetcher.get();
    source_ = base::MakeUnique<ContextualSuggestionsSource>(
        std::move(fetcher),
        base::MakeUnique<FakeCachedImageFetcher>(&pref_service_),
        std::unique_ptr<RemoteSuggestionsDatabase>());
  }

  FakeContextualSuggestionsFetcher* fetcher() { return fetcher_; }
  ContextualSuggestionsSource* source() { return source_.get(); }

 private:
  FakeContextualSuggestionsFetcher* fetcher_;
  base::MessageLoop message_loop_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<ContextualSuggestionsSource> source_;

  DISALLOW_COPY_AND_ASSIGN(ContextualSuggestionsSourceTest);
};

TEST_F(ContextualSuggestionsSourceTest, ShouldFetchContextualSuggestion) {
  MockFetchContextualSuggestionsCallback mock_suggestions_callback;
  const std::string kValidFromUrl = "http://some.url";
  const std::string kToUrl = "http://another.url";
  fetcher()->SetFakeResponse(Status::Success(), kToUrl);
  EXPECT_CALL(mock_suggestions_callback,
              Run(Property(&Status::IsSuccess, true), GURL(kValidFromUrl),
                  ElementsAre(AllOf(
                      Property(&ContentSuggestion::id,
                               Property(&ContentSuggestion::ID::category,
                                        Category::FromKnownCategory(
                                            KnownCategories::CONTEXTUAL))),
                      Property(&ContentSuggestion::url, GURL(kToUrl))))));
  source()->FetchContextualSuggestions(
      GURL(kValidFromUrl), mock_suggestions_callback.ToOnceCallback());
  base::RunLoop().RunUntilIdle();
}

TEST_F(ContextualSuggestionsSourceTest, ShouldCallbackOnEmptyResults) {
  MockFetchContextualSuggestionsCallback mock_suggestions_callback;
  const std::string kEmpty;
  fetcher()->SetFakeResponse(Status::Success(), kEmpty);
  EXPECT_CALL(mock_suggestions_callback,
              Run(Property(&Status::IsSuccess, true), GURL(kEmpty), IsEmpty()));
  source()->FetchContextualSuggestions(
      GURL(kEmpty), mock_suggestions_callback.ToOnceCallback());
  base::RunLoop().RunUntilIdle();
}

TEST_F(ContextualSuggestionsSourceTest, ShouldCallbackOnError) {
  MockFetchContextualSuggestionsCallback mock_suggestions_callback;
  const std::string kEmpty;
  fetcher()->SetFakeResponse(Status(StatusCode::TEMPORARY_ERROR, ""), kEmpty);
  EXPECT_CALL(
      mock_suggestions_callback,
      Run(Property(&Status::IsSuccess, false), GURL(kEmpty), IsEmpty()));
  source()->FetchContextualSuggestions(
      GURL(kEmpty), mock_suggestions_callback.ToOnceCallback());
  base::RunLoop().RunUntilIdle();
}

TEST_F(ContextualSuggestionsSourceTest, ShouldCallbackIfImageNotFound) {
  base::MockCallback<ImageFetchedCallback> mock_image_fetched_callback;
  const std::string kEmpty;
  ContentSuggestion::ID id(
      Category::FromKnownCategory(KnownCategories::CONTEXTUAL), kEmpty);
  EXPECT_CALL(mock_image_fetched_callback,
              Run(Property(&gfx::Image::IsEmpty, Eq(true))));
  source()->FetchContextualSuggestionImage(id,
                                           mock_image_fetched_callback.Get());
  base::RunLoop().RunUntilIdle();
}

TEST_F(ContextualSuggestionsSourceTest,
       ShouldFetchContextualSuggestionAndImage) {
  const std::string kValidFromUrl = "http://some.url";
  const std::string kToUrl = "http://another.url";
  const std::string kValidImageUrl = "http://some.url/image.png";
  fetcher()->SetFakeResponse(Status::Success(), kToUrl, kValidImageUrl);

  MockFetchContextualSuggestionsCallback mock_suggestions_callback;
  ContentSuggestion::ID id(
      Category::FromKnownCategory(KnownCategories::CONTEXTUAL), "");
  EXPECT_CALL(mock_suggestions_callback, Run(_, _, _))
      .WillOnce(SaveFirstIDFromContextualSuggestionsCallback(&id));
  source()->FetchContextualSuggestions(
      GURL(kValidFromUrl), mock_suggestions_callback.ToOnceCallback());
  base::RunLoop().RunUntilIdle();

  base::MockCallback<ImageFetchedCallback> mock_image_fetched_callback;
  EXPECT_CALL(mock_image_fetched_callback,
              Run(Property(&gfx::Image::IsEmpty, Eq(false))));
  source()->FetchContextualSuggestionImage(id,
                                           mock_image_fetched_callback.Get());
  base::RunLoop().RunUntilIdle();
}

}  // namespace ntp_snippets
