// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/remote/contextual_suggestions_fetcher.h"

#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/default_clock.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/ntp_snippets/category.h"
#include "components/signin/core/browser/access_token_fetcher.h"
#include "components/strings/grit/components_strings.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_status.h"
#include "ui/base/l10n/l10n_util.h"

using net::HttpRequestHeaders;
using net::URLFetcher;
using net::URLRequestContextGetter;

namespace ntp_snippets {

using internal::ContextualJsonRequest;
using internal::FetchResult;

namespace {

const char kContentSuggestionsApiScope[] =
    "https://www.googleapis.com/auth/chrome-content-suggestions";
const char kAuthorizationRequestHeaderFormat[] = "Bearer %s";

std::string FetchResultToString(FetchResult result) {
  switch (result) {
    case FetchResult::SUCCESS:
      return "OK";
    case FetchResult::URL_REQUEST_STATUS_ERROR:
      return "URLRequestStatus error";
    case FetchResult::HTTP_ERROR:
      return "HTTP error";
    case FetchResult::JSON_PARSE_ERROR:
      return "Received invalid JSON";
    case FetchResult::INVALID_SNIPPET_CONTENT_ERROR:
      return "Invalid / empty list.";
    case FetchResult::OAUTH_TOKEN_ERROR:
      return "Error in obtaining an OAuth2 access token.";
    case FetchResult::MISSING_API_KEY:
      return "No API key available.";
    case FetchResult::RESULT_MAX:
      break;
  }
  NOTREACHED();
  return "Unknown error";
}

Status FetchResultToStatus(FetchResult result) {
  switch (result) {
    case FetchResult::SUCCESS:
      return Status::Success();
    // Permanent errors occur if it is more likely that the error originated
    // from the client.
    case FetchResult::OAUTH_TOKEN_ERROR:
    case FetchResult::MISSING_API_KEY:
      return Status(StatusCode::PERMANENT_ERROR, FetchResultToString(result));
    // Temporary errors occur if it's more likely that the client behaved
    // correctly but the server failed to respond as expected.
    // TODO(fhorschig): Revisit HTTP_ERROR once the rescheduling was reworked.
    case FetchResult::HTTP_ERROR:
    case FetchResult::URL_REQUEST_STATUS_ERROR:
    case FetchResult::INVALID_SNIPPET_CONTENT_ERROR:
    case FetchResult::JSON_PARSE_ERROR:
      return Status(StatusCode::TEMPORARY_ERROR, FetchResultToString(result));
    case FetchResult::RESULT_MAX:
      break;
  }
  NOTREACHED();
  return Status(StatusCode::PERMANENT_ERROR, std::string());
}

std::string GetFetchEndpoint() {
  return "https://alpha-chromecontentsuggestions-pa.sandbox.googleapis.com/v1"
         "/publicdebate"
         "/getsuggestions";
}

// Creates snippets from dictionary values in |list| and adds them to
// |snippets|. Returns true on success, false if anything went wrong.
// |remote_category_id| is only used if |content_suggestions_api| is true.

bool AddSnippetsFromListValue(bool content_suggestions_api,
                              int remote_category_id,
                              const base::ListValue& list,
                              RemoteSuggestion::PtrVector* snippets) {
  for (const auto& value : list) {
    const base::DictionaryValue* dict = nullptr;
    if (!value.GetAsDictionary(&dict)) {
      return false;
    }

    std::string s;
    dict->GetAsString(&s);
    DVLOG(1) << "AddSnippetsFromListValue " << s;
    std::unique_ptr<RemoteSuggestion> snippet;
    snippet =
        RemoteSuggestion::CreateFromContextualSuggestionsDictionary(*dict);
    snippets->push_back(std::move(snippet));
  }
  return true;
}

}  // namespace

CategoryInfo BuildContextualSuggestionsCategoryInfo(
    const base::string16& title,
    bool allow_fetching_more_results) {
  ContentSuggestionsAdditionalAction action =
      allow_fetching_more_results ? ContentSuggestionsAdditionalAction::FETCH
                                  : ContentSuggestionsAdditionalAction::NONE;
  return CategoryInfo(
      title, ContentSuggestionsCardLayout::FULL_CARD, action,
      /*show_if_empty=*/false,
      // TODO(tschumann): The message for no-articles is likely wrong
      // and needs to be added to the stubby protocol if we want to
      // support it.
      l10n_util::GetStringUTF16(IDS_NTP_ARTICLE_SUGGESTIONS_SECTION_EMPTY));
}

ContextualSuggestionsFetcher::FetchedCategory::FetchedCategory(
    Category c,
    CategoryInfo&& info)
    : category(c), info(info) {}

ContextualSuggestionsFetcher::FetchedCategory::FetchedCategory(
    FetchedCategory&&) = default;

ContextualSuggestionsFetcher::FetchedCategory::~FetchedCategory() = default;

ContextualSuggestionsFetcher::FetchedCategory&
ContextualSuggestionsFetcher::FetchedCategory::operator=(FetchedCategory&&) =
    default;

// TODO(gaschler): remove unused language_model, user_classifier
ContextualSuggestionsFetcher::ContextualSuggestionsFetcher(
    SigninManagerBase* signin_manager,
    OAuth2TokenService* token_service,
    scoped_refptr<URLRequestContextGetter> url_request_context_getter,
    PrefService* pref_service,
    const ParseJSONCallback& parse_json_callback,
    const std::string& api_key)
    : signin_manager_(signin_manager),
      token_service_(token_service),
      url_request_context_getter_(std::move(url_request_context_getter)),
      parse_json_callback_(parse_json_callback),
      fetch_url_(GetFetchEndpoint()),
      api_key_(api_key),
      clock_(new base::DefaultClock()) {}

ContextualSuggestionsFetcher::~ContextualSuggestionsFetcher() = default;

const std::string& ContextualSuggestionsFetcher::GetLastStatusForDebugging()
    const {
  return last_status_;
}
const std::string& ContextualSuggestionsFetcher::GetLastJsonForDebugging()
    const {
  return last_fetch_json_;
}
const GURL& ContextualSuggestionsFetcher::GetFetchUrlForDebugging() const {
  return fetch_url_;
}

void ContextualSuggestionsFetcher::FetchContextualSuggestions(
    const GURL& url,
    SnippetsAvailableCallback callback) {
  ContextualJsonRequest::Builder builder;
  builder.SetParseJsonCallback(parse_json_callback_)
      .SetUrlRequestContextGetter(url_request_context_getter_)
      .SetContentUrl(url);

  pending_requests_.emplace(std::move(builder), std::move(callback));
  StartTokenRequest();
}

void ContextualSuggestionsFetcher::FetchAuthenticated(
    ContextualJsonRequest::Builder builder,
    SnippetsAvailableCallback callback,
    const std::string& oauth_access_token) {
  // TODO(jkrcal, treib): Add unit-tests for authenticated fetches.
  builder.SetUrl(fetch_url_)
      .SetAuthentication(signin_manager_->GetAuthenticatedAccountId(),
                         base::StringPrintf(kAuthorizationRequestHeaderFormat,
                                            oauth_access_token.c_str()));
  StartRequest(std::move(builder), std::move(callback));
}

void ContextualSuggestionsFetcher::StartRequest(
    ContextualJsonRequest::Builder builder,
    SnippetsAvailableCallback callback) {
  DVLOG(0) << "ContextualSuggestionsFetcher::StartRequest";
  std::unique_ptr<ContextualJsonRequest> request = builder.Build();
  ContextualJsonRequest* raw_request = request.get();
  raw_request->Start(base::BindOnce(
      &ContextualSuggestionsFetcher::JsonRequestDone, base::Unretained(this),
      std::move(request), std::move(callback)));
}

void ContextualSuggestionsFetcher::StartTokenRequest() {
  // If there is already an ongoing token request, just wait for that.
  if (token_fetcher_) {
    return;
  }

  OAuth2TokenService::ScopeSet scopes{kContentSuggestionsApiScope};
  token_fetcher_ = base::MakeUnique<AccessTokenFetcher>(
      "ntp_snippets", signin_manager_, token_service_, scopes,
      base::BindOnce(&ContextualSuggestionsFetcher::AccessTokenFetchFinished,
                     base::Unretained(this)));
}

void ContextualSuggestionsFetcher::AccessTokenFetchFinished(
    const GoogleServiceAuthError& error,
    const std::string& access_token) {
  // Delete the fetcher only after we leave this method (which is called from
  // the fetcher itself).
  DCHECK(token_fetcher_);
  std::unique_ptr<AccessTokenFetcher> token_fetcher_deleter(
      std::move(token_fetcher_));

  if (error.state() != GoogleServiceAuthError::NONE) {
    AccessTokenError(error);
    return;
  }

  DCHECK(!access_token.empty());

  while (!pending_requests_.empty()) {
    std::pair<ContextualJsonRequest::Builder, SnippetsAvailableCallback>
        builder_and_callback = std::move(pending_requests_.front());
    pending_requests_.pop();
    FetchAuthenticated(std::move(builder_and_callback.first),
                       std::move(builder_and_callback.second), access_token);
  }
}

void ContextualSuggestionsFetcher::AccessTokenError(
    const GoogleServiceAuthError& error) {
  DCHECK_NE(error.state(), GoogleServiceAuthError::NONE);

  LOG(WARNING) << "ContextualSuggestionsFetcher::AccessTokenError "
                  "Unable to get token: "
               << error.ToString();

  while (!pending_requests_.empty()) {
    std::pair<ContextualJsonRequest::Builder, SnippetsAvailableCallback>
        builder_and_callback = std::move(pending_requests_.front());

    FetchFinished(OptionalFetchedCategories(),
                  std::move(builder_and_callback.second),
                  FetchResult::OAUTH_TOKEN_ERROR,
                  /*error_details=*/
                  base::StringPrintf(" (%s)", error.ToString().c_str()));
    pending_requests_.pop();
  }
}

void ContextualSuggestionsFetcher::JsonRequestDone(
    std::unique_ptr<ContextualJsonRequest> request,
    SnippetsAvailableCallback callback,
    std::unique_ptr<base::Value> result,
    FetchResult status_code,
    const std::string& error_details) {
  DCHECK(request);

  DVLOG(0) << "ContextualSuggestionsFetcher::JsonRequestDone status_code="
           << static_cast<int>(status_code)
           << " error_details=" << error_details;
  last_fetch_json_ = request->GetResponseString();

  // TODO(gaschler): Add UMA metrics for fetch duration of the request

  if (!result) {
    FetchFinished(OptionalFetchedCategories(), std::move(callback), status_code,
                  error_details);
    return;
  }

  FetchedCategoriesVector categories;
  if (!JsonToSnippets(*result, &categories)) {
    DLOG(WARNING) << "Received invalid snippets: " << last_fetch_json_;
    FetchFinished(OptionalFetchedCategories(), std::move(callback),
                  FetchResult::INVALID_SNIPPET_CONTENT_ERROR, std::string());
    return;
  }

  FetchFinished(std::move(categories), std::move(callback),
                FetchResult::SUCCESS, std::string());
}

void ContextualSuggestionsFetcher::FetchFinished(
    OptionalFetchedCategories categories,
    SnippetsAvailableCallback callback,
    FetchResult fetch_result,
    const std::string& error_details) {
  DCHECK(fetch_result == FetchResult::SUCCESS || !categories.has_value());

  last_status_ = FetchResultToString(fetch_result) + error_details;

  DVLOG(1) << "Fetch finished: " << last_status_;

  std::move(callback).Run(FetchResultToStatus(fetch_result),
                          std::move(categories));
}

bool ContextualSuggestionsFetcher::JsonToSnippets(
    const base::Value& parsed,
    FetchedCategoriesVector* categories) {
  const base::DictionaryValue* top_dict = nullptr;
  if (!parsed.GetAsDictionary(&top_dict)) {
    return false;
  }

  // TODO: Use the remote category id to create the category.
  Category category = Category::FromKnownCategory(KnownCategories::CONTEXTUAL);
  const base::ListValue* categories_value = nullptr;
  if (!top_dict->GetList("categories", &categories_value)) {
    return false;
  }

  for (const auto& v : *categories_value) {
    std::string utf8_title("Contextual Suggestions");
    const base::DictionaryValue* category_value = nullptr;
    if (!(v.GetAsDictionary(&category_value))) {  //  &&
      return false;
    }

    RemoteSuggestion::PtrVector snippets;
    const base::ListValue* suggestions = nullptr;
    // Absence of a list of suggestions is treated as an empty list, which
    // is permissible.
    if (category_value->GetList("suggestions", &suggestions)) {
      int category_id =
          category.id() - (int)KnownCategories::REMOTE_CATEGORIES_OFFSET;
      if (!AddSnippetsFromListValue(true, category_id, *suggestions,
                                    &snippets)) {
        return false;
      }
    }

    // Add fetched Category
    categories->push_back(
        FetchedCategory(category, BuildContextualSuggestionsCategoryInfo(
                                      base::UTF8ToUTF16(utf8_title),
                                      /*allow_fetching_more_results=*/false)));
    categories->back().snippets = std::move(snippets);
  }

  return true;
}

}  // namespace ntp_snippets
