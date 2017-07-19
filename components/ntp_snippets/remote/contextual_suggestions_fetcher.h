// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_REMOTE_CONTEXTUAL_SUGGESTIONS_FETCHER_H_
#define COMPONENTS_NTP_SNIPPETS_REMOTE_CONTEXTUAL_SUGGESTIONS_FETCHER_H_

#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/optional.h"
#include "base/time/clock.h"
#include "components/ntp_snippets/category.h"
#include "components/ntp_snippets/category_info.h"
#include "components/ntp_snippets/remote/contextual_json_request.h"
#include "components/ntp_snippets/remote/remote_suggestion.h"
#include "components/ntp_snippets/status.h"
#include "net/url_request/url_request_context_getter.h"

class AccessTokenFetcher;
class OAuth2TokenService;
class PrefService;
class SigninManagerBase;

namespace ntp_snippets {

// Fetches contextual suggestions for the NTP from the server.
class ContextualSuggestionsFetcher {
 public:
  struct FetchedCategory {
    Category category;
    CategoryInfo info;
    RemoteSuggestion::PtrVector snippets;

    FetchedCategory(Category c, CategoryInfo&& info);
    FetchedCategory(FetchedCategory&&);             // = default, in .cc
    ~FetchedCategory();                             // = default, in .cc
    FetchedCategory& operator=(FetchedCategory&&);  // = default, in .cc
  };
  using FetchedCategoriesVector = std::vector<FetchedCategory>;
  using OptionalFetchedCategories = base::Optional<FetchedCategoriesVector>;

  // |snippets| contains parsed snippets if a fetch succeeded. If problems
  // occur, |snippets| contains no value (no actual vector in base::Optional).
  // Error details can be retrieved using last_status().
  using SnippetsAvailableCallback =
      base::OnceCallback<void(Status status,
                              OptionalFetchedCategories fetched_categories)>;

  ContextualSuggestionsFetcher(
      SigninManagerBase* signin_manager,
      OAuth2TokenService* token_service,
      scoped_refptr<net::URLRequestContextGetter> url_request_context_getter,
      PrefService* pref_service,
      const ParseJSONCallback& parse_json_callback,
      const std::string& api_key);
  ~ContextualSuggestionsFetcher();

  void FetchContextualSuggestions(const GURL& url,
                                  SnippetsAvailableCallback callback);

  const std::string& GetLastStatusForDebugging() const;
  const std::string& GetLastJsonForDebugging() const;
  const GURL& GetFetchUrlForDebugging() const;

  // Overrides internal clock for testing purposes.
  void SetClockForTesting(std::unique_ptr<base::Clock> clock) {
    clock_ = std::move(clock);
  }

 private:
  void FetchAuthenticated(internal::ContextualJsonRequest::Builder builder,
                          SnippetsAvailableCallback callback,
                          const std::string& oauth_access_token);
  void StartRequest(internal::ContextualJsonRequest::Builder builder,
                    SnippetsAvailableCallback callback);

  void StartTokenRequest();

  void AccessTokenFetchFinished(const GoogleServiceAuthError& error,
                                const std::string& access_token);
  void AccessTokenError(const GoogleServiceAuthError& error);

  void JsonRequestDone(std::unique_ptr<internal::ContextualJsonRequest> request,
                       SnippetsAvailableCallback callback,
                       std::unique_ptr<base::Value> result,
                       internal::FetchResult status_code,
                       const std::string& error_details);
  void FetchFinished(OptionalFetchedCategories categories,
                     SnippetsAvailableCallback callback,
                     internal::FetchResult status_code,
                     const std::string& error_details);

  bool JsonToSnippets(
      const base::Value& parsed,
      ContextualSuggestionsFetcher::FetchedCategoriesVector* categories);

  // Authentication for signed-in users.
  SigninManagerBase* signin_manager_;
  OAuth2TokenService* token_service_;

  std::unique_ptr<AccessTokenFetcher> token_fetcher_;

  // Holds the URL request context.
  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;

  // Stores requests that wait for an access token.
  std::queue<std::pair<internal::ContextualJsonRequest::Builder,
                       SnippetsAvailableCallback>>
      pending_requests_;

  const ParseJSONCallback parse_json_callback_;

  // API endpoint for fetching contextual suggestions.
  const GURL fetch_url_;

  // API key to use for non-authenticated requests.
  const std::string api_key_;

  // Allow for an injectable clock for testing.
  std::unique_ptr<base::Clock> clock_;

  // Info on the last finished fetch.
  std::string last_status_;
  std::string last_fetch_json_;

  DISALLOW_COPY_AND_ASSIGN(ContextualSuggestionsFetcher);
};

}  // namespace ntp_snippets

#endif  // COMPONENTS_NTP_SNIPPETS_REMOTE_CONTEXTUAL_SUGGESTIONS_FETCHER_H_
