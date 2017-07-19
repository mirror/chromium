// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OMNIBOX_BROWSER_CONTEXTUAL_SUGGESTIONS_SERVICE_H_
#define COMPONENTS_OMNIBOX_BROWSER_CONTEXTUAL_SUGGESTIONS_SERVICE_H_

#include <memory>
#include <string>

#include "components/keyed_service/core/keyed_service.h"
#include "components/signin/core/browser/access_token_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

class OAuth2TokenService;
class SigninManagerBase;
class TemplateURLService;

// A service to fetch suggestions from a remote endpoint given a context.
class ContextualSuggestionsService : public KeyedService {
 public:
  // signin_manager, token_service and request_context must NOT be nullptr.
  // template_url_service may be null, but it may disable some services.
  ContextualSuggestionsService(SigninManagerBase* signin_manager,
                               OAuth2TokenService* token_service,
                               TemplateURLService* template_url_service,
                               net::URLRequestContextGetter* request_context);

  ~ContextualSuggestionsService() override;

  using ContextualSuggestionsCallback =
      base::OnceCallback<void(std::unique_ptr<net::URLFetcher> fetcher)>;

  // Creates a fetch request for contextual suggestions for |current_url|.
  //
  // |fetcher_delegate| is used to create a fetcher that is used to perform a
  // network request. It uses a number of signals to create the fetcher,
  // including field trial parameters and experimental parameters, and it may
  // return an invalid fetcher.
  //
  // |callback| is called with the fetcher produced by the |fetcher_delegate| as
  // an argument if a network response is finally obtained. Otherwise |callback|
  // is called with a nullptr as argument if:
  //   * The fetcher obtained from the fetcher_delegate is invalid.
  //   * The service is waiting for an authentication token.
  //   * The authentication token had any issues.
  //
  // This method sends a request for an OAuth2 token and temporarily
  // instantiates token_fetcher_.
  void CreateContextualSuggestionsRequest(
      const std::string& current_url,
      net::URLFetcherDelegate* fetcher_delegate,
      ContextualSuggestionsCallback callback);

 private:
  // Returns a URL representing the address of the server where the zero suggest
  // request is being sent.
  // It uses the experimental suggest URL if |experimental| is true.
  GURL ZeroSuggestURL(const std::string& current_url,
                      bool is_experimental) const;

  // Returns true if the folowing conditions are valid:
  // * The |current_url| is non-empty.
  // * The |default_provider| is Google.
  // * The user is in the ZeroSuggestRedirectToChrome field trial.
  // * The field trial provides a valid server address where the suggest request
  //   is redirected.
  // * The suggest request is over HTTPS. This avoids leaking the current page
  //   URL or personal data in unencrypted network traffic.
  // Note: these checks are in addition to CanSendUrl() on the default
  // contextual suggestion URL.
  bool UseExperimentalZeroSuggestSuggestions(
      const std::string& current_url) const;

  // Returns a URL representing the address of the server where the zero suggest
  // requests are being redirected when serving experimental suggestions.
  static GURL ExperimentalZeroSuggestURL(const std::string& current_url);

  // Creates an HTTP GET request for contextual suggestions. The return fetcher
  // does not include a header corresponding to an authorization token.
  std::unique_ptr<net::URLFetcher> CreateRequest(
      const std::string& current_url,
      bool is_experimental,
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

  // Helper for fetching OAuth2 access tokens. This is non-null when an access
  // token request is currently in progress.
  std::unique_ptr<AccessTokenFetcher> token_fetcher_;

  DISALLOW_COPY_AND_ASSIGN(ContextualSuggestionsService);
};

#endif  // COMPONENTS_OMNIBOX_BROWSER_CONTEXTUAL_SUGGESTIONS_SERVICE_H_
