// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OMNIBOX_BROWSER_CONTEXTUAL_SUGGESTIONS_SERVICE_H_
#define COMPONENTS_OMNIBOX_BROWSER_CONTEXTUAL_SUGGESTIONS_SERVICE_H_

#include <memory>
#include <string>

#include "base/feature_list.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/search_engines/template_url_service.h"
#include "components/signin/core/browser/access_token_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

class OAuth2TokenService;
class SigninManagerBase;

namespace contextual_suggestions {

class ContextualSuggestionsService : public KeyedService {
 public:
  ContextualSuggestionsService(SigninManagerBase* signin_manager,
                               OAuth2TokenService* token_service,
                               TemplateURLService* template_url_service,
                               net::URLRequestContextGetter* request_context);

  ~ContextualSuggestionsService() override;

  using ContextualSuggestionsCallback =
      base::OnceCallback<void(std::unique_ptr<net::URLFetcher> fetcher)>;

  // Creates a fetch request for contextual suggestions for |visited_url|, with
  // |fetcher_delegate| as the URLFetcherDelegate that will process the response
  // of the fetcher.
  //
  // * If the URL to send the request to, which is constructed using field trial
  //   parameters, is invalid, |callback| is executed synchronously witha
  //   nullptr for |fetcher|.
  // * If the service is not using experimental suggestions, |callback| is
  //   executed synchronously with a valid |fetcher| as a parameter.
  // * If the service is supposed to use experimental suggestions and it is
  // waiting for an authentication token,  |callback| is executed synchronously
  // with a nullptr for |fetcher|. * If the service is supposed to use
  // experimental suggestions and it is not waiting for an authentication token,
  // a request for an auth token is made asynchronously. After the token is
  // obtained and attached to the |fetcher|, |callback| is called with a valid
  // |fetcher| as a parameter.
  void CreateContextualSuggestionsRequest(
      const std::string& visited_url,
      net::URLFetcherDelegate* fetcher_delegate,
      ContextualSuggestionsCallback callback);

 private:
  // Returns true if the folowing conditions are valid:
  // * The |visited_url| is non-empty.
  // * The |default_provider| is Google.
  // * The user is in the ZeroSuggestRedirectToChrome field trial.
  // * The field trial provides a valid server address where the suggest request
  //   is redirected.
  // * The suggest request is over HTTPS. This avoids leaking the current page
  //   URL or personal data in unencrypted network traffic.
  // Note: these checks are in addition to CanSendUrl() on the default
  // contextual suggestion URL.
  bool UseExperimentalZeroSuggestSuggestions(
      const std::string& visited_url) const;

  // Returns a string representing the address of the server where the zero
  // suggest requests are being redirected when serving experimental
  // suggestions.
  static GURL ExperimentalZeroSuggestURL(const std::string& visited_url);

  // Returns a string representing the address of the server where the zero
  // suggest request is being sent.
  GURL ZeroSuggestURL(const std::string& visited_url, bool experimental) const;

  // Creates an HTTP Get request for contextual suggestions. The return value
  // does not include a header corresponding to an authorization token.
  std::unique_ptr<net::URLFetcher> CreateRequest(
      const std::string& visited_url,
      bool experimental,
      net::URLFetcherDelegate* fetcher_delegate) const;

  // Called when an access token request completes (successfully or not).
  void AccessTokenAvailable(std::unique_ptr<net::URLFetcher> fetcher,
                            ContextualSuggestionsCallback callback,
                            const GoogleServiceAuthError& error,
                            const std::string& access_token);

  net::URLRequestContextGetter* request_context_;
  SigninManagerBase* signin_manager_;
  TemplateURLService* template_url_service_;
  OAuth2TokenService* token_service_;

  // Helper for fetching OAuth2 access tokens. This is non-null iff an access
  // token request is currently in progress.
  std::unique_ptr<AccessTokenFetcher> token_fetcher_;
};

}  // namespace contextual_suggestions

#endif  // COMPONENTS_OMNIBOX_BROWSER_CONTEXTUAL_SUGGESTIONS_SERVICE_H_
