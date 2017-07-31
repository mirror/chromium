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
#include "components/image_fetcher/core/image_fetcher_impl.h"
#include "components/ntp_snippets/category_info.h"
#include "components/ntp_snippets/content_suggestion.h"
#include "components/ntp_snippets/remote/contextual_suggestions_fetcher.h"
#include "components/ntp_snippets/remote/json_to_categories.h"
#include "components/ntp_snippets/remote/proto/ntp_snippets.pb.h"
#include "components/ntp_snippets/remote/remote_suggestion.h"
#include "components/ntp_snippets/remote/remote_suggestions_database.h"
#include "components/ntp_snippets/remote/remote_suggestions_provider_impl.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Eq;
using testing::Mock;
using testing::Property;

namespace ntp_snippets {

namespace {

const char kFromURL[] = "http://localhost";
const char kSuggestionURL[] = "http://url.test";

MATCHER(IsSingleContextualSuggestion,
        "is a list with a single contextual suggestion") {
  std::vector<ContentSuggestion>& suggestions = *arg;
  if (suggestions.size() != 1) {
    *result_listener << "expected single suggestion.";
    return false;
  }
  if (suggestions[0].id().category() !=
      Category::FromKnownCategory(KnownCategories::CONTEXTUAL)) {
    *result_listener << "expected category CONTEXTUAL.";
    return false;
  }
  if (suggestions[0].url() != GURL(kSuggestionURL)) {
    *result_listener << "expected suggestion URL " << kSuggestionURL
                     << " got URL " << suggestions[0].url();
    return false;
  }
  return true;
}

std::unique_ptr<RemoteSuggestion> CreateTestSuggestion() {
  SnippetProto proto;
  proto.add_ids(kFromURL);
  auto* source = proto.add_sources();
  source->set_url(kSuggestionURL);
  return RemoteSuggestion::CreateFromProto(proto);
}

// Always fetches one valid RemoteSuggestion
class FakeContextualSuggestionsFetcher : public ContextualSuggestionsFetcher {
  void FetchContextualSuggestions(
      const GURL& url,
      SuggestionsAvailableCallback callback) override {
    OptionalSuggestions suggestions = RemoteSuggestion::PtrVector();
    suggestions->push_back(CreateTestSuggestion());
    std::move(callback).Run(Status::Success(), std::move(suggestions));
  }

  MOCK_CONST_METHOD0(GetLastStatusForTesting, std::string&());
  MOCK_CONST_METHOD0(GetLastJsonForTesting, std::string&());
  MOCK_CONST_METHOD0(GetFetchUrlForTesting, GURL&());
};

class MockFetchContextualSuggestionsCallback {
 public:
  // Workaround for gMock's lack of support for movable arguments.
  void WrappedRun(Status status,
                  const GURL& url,
                  std::vector<ContentSuggestion> suggestions) {
    Run(status, url, &suggestions);
  }

  ContextualSuggestionsSource::FetchContextualSuggestionsCallback
  ToMovableCallback() {
    return base::BindOnce(&MockFetchContextualSuggestionsCallback::WrappedRun,
                          base::Unretained(this));
  }

  MOCK_METHOD3(Run,
               void(Status status_code,
                    const GURL& url,
                    std::vector<ContentSuggestion>* suggestions));
};

}  // namespace

class ContextualSuggestionsSourceTest : public testing::Test {
 public:
  ContextualSuggestionsSourceTest() {
    source_ = base::MakeUnique<ContextualSuggestionsSource>(
        base::MakeUnique<FakeContextualSuggestionsFetcher>(),
        std::unique_ptr<CachedImageFetcher>(),
        std::unique_ptr<RemoteSuggestionsDatabase>());
  }

  ContextualSuggestionsSource* source() { return source_.get(); }

 private:
  base::MessageLoop message_loop_;
  std::unique_ptr<ContextualSuggestionsSource> source_;

  DISALLOW_COPY_AND_ASSIGN(ContextualSuggestionsSourceTest);
};

TEST_F(ContextualSuggestionsSourceTest, ShouldFetchContextualSuggestion) {
  MockFetchContextualSuggestionsCallback mock_suggestions_callback;
  EXPECT_CALL(mock_suggestions_callback,
              Run(Property(&Status::IsSuccess, true), GURL(kFromURL),
                  IsSingleContextualSuggestion()));
  source()->FetchContextualSuggestions(
      GURL(kFromURL), mock_suggestions_callback.ToMovableCallback());
  base::RunLoop().RunUntilIdle();
}

}  // namespace ntp_snippets
