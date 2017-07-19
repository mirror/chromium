// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/remote/contextual_suggestions_fetcher.h"

#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/memory/ptr_util.h"
#include "base/optional.h"
#include "base/test/test_mock_time_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "components/ntp_snippets/category.h"
#include "components/ntp_snippets/remote/test_utils.h"
#include "components/prefs/testing_pref_service.h"
#include "components/signin/core/browser/fake_profile_oauth2_token_service.h"
#include "components/signin/core/browser/fake_signin_manager.h"
#include "google_apis/gaia/fake_oauth2_token_service_delegate.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_snippets {

namespace {

using testing::_;
using testing::AllOf;
using testing::Eq;
using testing::Field;
using testing::IsEmpty;
using testing::Property;
using testing::StartsWith;

const char kAPIKey[] = "fakeAPIkey";
const char kTestEmail[] = "foo@bar.com";
const GURL kValidURL = GURL("http://valid-url.test");

MATCHER(IsEmptySuggestionsList, "is an empty list of suggestions") {
  ContextualSuggestionsFetcher::OptionalFetchedCategories& fetched_categories =
      *arg;
  if (!fetched_categories || fetched_categories->size() < 1) {
    *result_listener << "expected a category.";
    return false;
  }
  auto category = fetched_categories->begin();
  if (category->snippets.size() != 0) {
    *result_listener << "expected empty suggestions, got: "
                     << category->snippets.size();
    return false;
  }
  return true;
}

MATCHER_P(IsSingleSuggestion,
          url,
          "is a list with the single suggestion %(url)s") {
  ContextualSuggestionsFetcher::OptionalFetchedCategories& fetched_categories =
      *arg;
  if (!fetched_categories) {
    *result_listener << "got empty categories.";
    return false;
  }
  if (fetched_categories->size() != 1) {
    *result_listener << "expected single category.";
    return false;
  }
  auto category = fetched_categories->begin();
  if (category->snippets.size() != 1) {
    *result_listener << "expected single snippet, got: "
                     << category->snippets.size();
    return false;
  }
  if (category->snippets[0]->url().spec() != url) {
    *result_listener << "unexpected url, got: "
                     << category->snippets[0]->url().spec();
    return false;
  }
  return true;
}

class MockSnippetsAvailableCallback {
 public:
  // Workaround for gMock's lack of support for movable arguments.
  void WrappedRun(Status status,
                  ContextualSuggestionsFetcher::OptionalFetchedCategories
                      fetched_categories) {
    Run(status, &fetched_categories);
  }

  MOCK_METHOD2(Run,
               void(Status status,
                    ContextualSuggestionsFetcher::OptionalFetchedCategories*
                        fetched_categories));
};

void ParseJson(const std::string& json,
               const SuccessCallback& success_callback,
               const ErrorCallback& error_callback) {
  base::JSONReader json_reader;
  std::unique_ptr<base::Value> value = json_reader.ReadToValue(json);
  if (value) {
    success_callback.Run(std::move(value));
  } else {
    error_callback.Run(json_reader.GetErrorMessage());
  }
}

}  // namespace

class ContextualSuggestionsFetcherTest : public testing::Test {
 public:
  ContextualSuggestionsFetcherTest()
      : fake_url_fetcher_factory_(new net::FakeURLFetcherFactory(nullptr)),
        mock_task_runner_(new base::TestMockTimeTaskRunner()),
        mock_task_runner_handle_(mock_task_runner_) {
    scoped_refptr<net::TestURLRequestContextGetter> request_context_getter =
        new net::TestURLRequestContextGetter(mock_task_runner_.get());
    fake_token_service_ = base::MakeUnique<FakeProfileOAuth2TokenService>(
        base::MakeUnique<FakeOAuth2TokenServiceDelegate>(
            request_context_getter.get()));
    fetcher_ = base::MakeUnique<ContextualSuggestionsFetcher>(
        test_utils_.fake_signin_manager(), fake_token_service_.get(),
        std::move(request_context_getter), test_utils_.pref_service(),
        base::Bind(&ParseJson), kAPIKey);
    fetcher_->SetClockForTesting(mock_task_runner_->GetMockClock());
  }

  void FastForwardUntilNoTasksRemain() {
    mock_task_runner_->FastForwardUntilNoTasksRemain();
  }

  void InitializeFakeCredentials() {
    SignIn();
    IssueRefreshToken();
  }

  void IssueRefreshToken() {
    fake_token_service_->GetDelegate()->UpdateCredentials(kTestEmail, "token");
  }

  void IssueOAuth2Token() {
    fake_token_service_->IssueAllTokensForAccount(kTestEmail, "access_token",
                                                  base::Time::Max());
  }

  void SetFakeResponse(const std::string& response_data,
                       net::HttpStatusCode response_code,
                       net::URLRequestStatus::Status status) {
    fake_url_fetcher_factory_->SetFakeResponse(
        fetcher_->GetFetchUrlForDebugging(), response_data, response_code,
        status);
  }

  void SignIn() {
#if defined(OS_CHROMEOS)
    test_utils_.fake_signin_manager()->SignIn(kTestEmail);
#else
    test_utils_.fake_signin_manager()->SignIn(kTestEmail, "user", "password");
#endif
  }

  ContextualSuggestionsFetcher::SnippetsAvailableCallback
  ToSnippetsAvailableCallback(MockSnippetsAvailableCallback* callback) {
    return base::BindOnce(&MockSnippetsAvailableCallback::WrappedRun,
                          base::Unretained(callback));
  }

  ContextualSuggestionsFetcher& fetcher() { return *fetcher_; }
  MockSnippetsAvailableCallback& mock_snippets_available_callback() {
    return mock_snippets_available_callback_;
  }

 private:
  std::unique_ptr<FakeProfileOAuth2TokenService> fake_token_service_;
  std::unique_ptr<net::FakeURLFetcherFactory> fake_url_fetcher_factory_;
  std::unique_ptr<ContextualSuggestionsFetcher> fetcher_;
  MockSnippetsAvailableCallback mock_snippets_available_callback_;
  scoped_refptr<base::TestMockTimeTaskRunner> mock_task_runner_;
  base::ThreadTaskRunnerHandle mock_task_runner_handle_;
  test::RemoteSuggestionsTestUtils test_utils_;

  DISALLOW_COPY_AND_ASSIGN(ContextualSuggestionsFetcherTest);
};

TEST_F(ContextualSuggestionsFetcherTest, CreateFetcher) {
  FastForwardUntilNoTasksRemain();
  EXPECT_THAT(fetcher().GetLastStatusForDebugging(), IsEmpty());
}

TEST_F(ContextualSuggestionsFetcherTest, ShouldFetchSuccessfully) {
  InitializeFakeCredentials();
  const std::string kValidResponseData =
      "{\"categories\" : [{"
      "  \"id\": 0,"
      "  \"suggestions\" : [{"
      "    \"title\" : \"Title\","
      "    \"summary\" : \"...\","
      "    \"url\" : \"http://localhost/foobar\","
      "    \"imageUrl\" : \"http://localhost/foobar.jpg\""
      "  }]"
      "}]}";
  SetFakeResponse(/*response_data=*/kValidResponseData, net::HTTP_OK,
                  net::URLRequestStatus::SUCCESS);
  EXPECT_CALL(mock_snippets_available_callback(),
              Run(Property(&Status::IsSuccess, true),
                  IsSingleSuggestion("http://localhost/foobar")));

  fetcher().FetchContextualSuggestions(
      kValidURL,
      ToSnippetsAvailableCallback(&mock_snippets_available_callback()));
  IssueOAuth2Token();
  FastForwardUntilNoTasksRemain();
  EXPECT_THAT(fetcher().GetLastStatusForDebugging(), Eq("OK"));
  EXPECT_THAT(fetcher().GetLastJsonForDebugging(), Eq(kValidResponseData));
}

TEST_F(ContextualSuggestionsFetcherTest, EmptySuggestionsList) {
  InitializeFakeCredentials();
  const std::string kValidEmptyCategoryResponseData =
      "{\"categories\" : [{"
      "  \"id\": 0,"
      "  \"suggestions\": []"
      "}]}";
  SetFakeResponse(/*response_data=*/kValidEmptyCategoryResponseData,
                  net::HTTP_OK, net::URLRequestStatus::SUCCESS);
  EXPECT_CALL(mock_snippets_available_callback(),
              Run(Property(&Status::IsSuccess, true),
                  /*fetched_categories=*/IsEmptySuggestionsList()));

  fetcher().FetchContextualSuggestions(
      kValidURL,
      ToSnippetsAvailableCallback(&mock_snippets_available_callback()));
  IssueOAuth2Token();
  FastForwardUntilNoTasksRemain();
  EXPECT_THAT(fetcher().GetLastStatusForDebugging(), Eq("OK"));
  EXPECT_THAT(fetcher().GetLastJsonForDebugging(),
              Eq(kValidEmptyCategoryResponseData));
}

TEST_F(ContextualSuggestionsFetcherTest, ReportErrorForEmptyResponseData) {
  InitializeFakeCredentials();
  SetFakeResponse(/*response_data=*/std::string(), net::HTTP_NOT_FOUND,
                  net::URLRequestStatus::SUCCESS);
  EXPECT_CALL(mock_snippets_available_callback(),
              Run(Property(&Status::IsSuccess, false), _));

  fetcher().FetchContextualSuggestions(
      kValidURL,
      ToSnippetsAvailableCallback(&mock_snippets_available_callback()));
  IssueOAuth2Token();
  FastForwardUntilNoTasksRemain();
}

TEST_F(ContextualSuggestionsFetcherTest, InvalidResponseData) {
  InitializeFakeCredentials();
  const std::string kInvalidResponseData = "{ \"recos\": []";
  SetFakeResponse(/*response_data=*/kInvalidResponseData, net::HTTP_OK,
                  net::URLRequestStatus::SUCCESS);
  EXPECT_CALL(mock_snippets_available_callback(),
              Run(Field(&Status::code, StatusCode::TEMPORARY_ERROR),
                  /*fetched_categories=*/Property(
                      &ContextualSuggestionsFetcher::OptionalFetchedCategories::
                          has_value,
                      false)))
      .Times(1);

  fetcher().FetchContextualSuggestions(
      kValidURL,
      ToSnippetsAvailableCallback(&mock_snippets_available_callback()));
  IssueOAuth2Token();
  FastForwardUntilNoTasksRemain();
  EXPECT_THAT(fetcher().GetLastStatusForDebugging(),
              StartsWith("Received invalid JSON (error "));
  EXPECT_THAT(fetcher().GetLastJsonForDebugging(), Eq(kInvalidResponseData));
}

}  // namespace ntp_snippets
