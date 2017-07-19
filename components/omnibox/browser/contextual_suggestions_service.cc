// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/contextual_suggestions_service.h"

#include "base/feature_list.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/stringprintf.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "components/omnibox/browser/omnibox_field_trial.h"
#include "components/search_engines/template_url_service.h"
#include "components/variations/net/variations_http_headers.h"
#include "net/base/escape.h"
#include "net/base/load_flags.h"
#include "net/http/http_response_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"

namespace {
// Server address for the experimental suggestions service.
const char kExperimentalServerAddress[] =
    "https://cuscochromeextension-pa.googleapis.com/v1/zerosuggestions";

// Parameter name used by the experiment redirecting Zero Suggestion requests
// to a service provided by the Chrome team.
const char kZeroSuggestRedirectToChromeExperimentIdParam[] =
    "ZeroSuggestRedirectToChromeExperimentID";

}  // namespace

ContextualSuggestionsService::ContextualSuggestionsService(
    SigninManagerBase* signin_manager,
    OAuth2TokenService* token_service,
    TemplateURLService* template_url_service,
    net::URLRequestContextGetter* request_context)
    : request_context_(request_context),
      signin_manager_(signin_manager),
      template_url_service_(template_url_service),
      token_service_(token_service) {}

ContextualSuggestionsService::~ContextualSuggestionsService() {}

bool ContextualSuggestionsService::UseExperimentalZeroSuggestSuggestions(
    const std::string& current_url) const {
  if (current_url.empty()) {
    return false;
  }

  if (!base::FeatureList::IsEnabled(omnibox::kZeroSuggestRedirectToChrome) ||
      template_url_service_ == nullptr) {
    return false;
  }

  // Check that the default search engine is Google.
  const TemplateURL& default_provider_url =
      *template_url_service_->GetDefaultSearchProvider();
  const SearchTermsData& search_terms_data =
      template_url_service_->search_terms_data();
  if (default_provider_url.GetEngineType(search_terms_data) !=
      SEARCH_ENGINE_GOOGLE) {
    return false;
  }

  // Check that the suggest URL for redirect to chrome field trial is valid.
  const GURL suggest_url = ExperimentalZeroSuggestURL(current_url);
  if (!suggest_url.is_valid()) {
    return false;
  }

  // Check that the suggest URL for redirect to chrome is HTTPS.
  return suggest_url.SchemeIsCryptographic();
}

void ContextualSuggestionsService::CreateContextualSuggestionsRequest(
    const std::string& current_url,
    net::URLFetcherDelegate* fetcher_delegate,
    ContextualSuggestionsCallback callback) {
  DCHECK(signin_manager_);

  const bool is_experimental =
      UseExperimentalZeroSuggestSuggestions(current_url);
  std::unique_ptr<net::URLFetcher> fetcher =
      CreateRequest(current_url, is_experimental, fetcher_delegate);
  if (is_experimental) {
    // Skip this request if still waiting for oauth2 token.
    if (token_fetcher_) {
      std::move(callback).Run(nullptr);
      return;
    }
    // Create the oauth2 token fetcher.
    OAuth2TokenService::ScopeSet scopes{
        "https://www.googleapis.com/auth/cusco-chrome-extension"};
    token_fetcher_ = base::MakeUnique<AccessTokenFetcher>(
        "contextual_suggestions_service", signin_manager_, token_service_,
        scopes,
        base::BindOnce(&ContextualSuggestionsService::AccessTokenAvailable,
                       base::Unretained(this), std::move(fetcher),
                       std::move(callback)));
    return;
  }

  std::move(callback).Run(std::move(fetcher));
}

// static
GURL ContextualSuggestionsService::ExperimentalZeroSuggestURL(
    const std::string& current_url) {
  const std::string server_address(kExperimentalServerAddress);
  const int experiment_id = base::GetFieldTrialParamByFeatureAsInt(
      omnibox::kZeroSuggestRedirectToChrome,
      kZeroSuggestRedirectToChromeExperimentIdParam, /*default_value=*/-1);
  const std::string additional_parameters =
      experiment_id >= 0
          ? base::StringPrintf("&experiment_id=%d", experiment_id)
          : "&";
  return GURL(server_address + "/url=" + net::EscapePath(current_url) +
              additional_parameters);
}

GURL ContextualSuggestionsService::ZeroSuggestURL(
    const std::string& current_url,
    bool is_experimental) const {
  if (is_experimental) {
    return ExperimentalZeroSuggestURL(current_url);
  }

  const TemplateURLRef& suggestion_url_ref =
      template_url_service_->GetDefaultSearchProvider()->suggestions_url_ref();
  const SearchTermsData& search_terms_data =
      template_url_service_->search_terms_data();
  base::string16 prefix;
  TemplateURLRef::SearchTermsArgs search_term_args(prefix);
  if (!current_url.empty()) {
    search_term_args.current_page_url = current_url;
  }
  return GURL(suggestion_url_ref.ReplaceSearchTerms(search_term_args,
                                                    search_terms_data));
}

std::unique_ptr<net::URLFetcher> ContextualSuggestionsService::CreateRequest(
    const std::string& current_url,
    bool is_experimental,
    net::URLFetcherDelegate* fetcher_delegate) const {
  if (current_url.empty())
    is_experimental = false;
  const GURL suggest_url = ZeroSuggestURL(current_url, is_experimental);
  DCHECK(suggest_url.is_valid());

  net::NetworkTrafficAnnotationTag annotation_tag =
      is_experimental
          ? net::DefineNetworkTrafficAnnotation(
                "omnibox_zerosuggest_experimental", R"(
        semantics {
          sender: "Omnibox"
          description:
            "When the user focuses the omnibox, Chrome can provide search or "
            "navigation suggestions from the default search provider in the "
            "omnibox dropdown, based on the current page URL.\n"
            "This is limited to users whose default search engine is Google, "
            "as no other search engines currently support this kind of "
            "suggestion."
          trigger: "The omnibox receives focus."
          data: "The user's OAuth2 credentials and the URL of the current page."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: false
          setting:
            "Users can control this feature via the 'Use a prediction service "
            "to help complete searches and URLs typed in the address bar' "
            "settings under 'Privacy'. The feature is enabled by default."
          chrome_policy {
            SearchSuggestEnabled {
                policy_options {mode: MANDATORY}
                SearchSuggestEnabled: false
            }
          }
        })")
          : net::DefineNetworkTrafficAnnotation("omnibox_zerosuggest", R"(
        semantics {
          sender: "Omnibox"
          description:
            "When the user focuses the omnibox, Chrome can provide search or "
            "navigation suggestions from the default search provider in the "
            "omnibox dropdown, based on the current page URL.\n"
            "This is limited to users whose default search engine is Google, "
            "as no other search engines currently support this kind of "
            "suggestion."
          trigger: "The omnibox receives focus."
          data: "The URL of the current page."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: true
          cookies_store: "user"
          setting:
            "Users can control this feature via the 'Use a prediction service "
            "to help complete searches and URLs typed in the address bar' "
            "settings under 'Privacy'. The feature is enabled by default."
          chrome_policy {
            SearchSuggestEnabled {
                policy_options {mode: MANDATORY}
                SearchSuggestEnabled: false
            }
          }
        })");

  const int kFetcherID = 1;
  std::unique_ptr<net::URLFetcher> fetcher =
      net::URLFetcher::Create(kFetcherID, suggest_url, net::URLFetcher::GET,
                              fetcher_delegate, annotation_tag);
  fetcher->SetRequestContext(request_context_);
  data_use_measurement::DataUseUserData::AttachToFetcher(
      fetcher.get(), data_use_measurement::DataUseUserData::OMNIBOX);

  // Add Chrome experiment state to the request headers.
  net::HttpRequestHeaders headers;
  // Note: It's OK to pass |is_signed_in| false if it's unknown, as it does
  // not affect transmission of experiments coming from the variations server.
  variations::AppendVariationHeaders(fetcher->GetOriginalURL(), false, false,
                                     /*is_signed_in=*/false, &headers);
  for (net::HttpRequestHeaders::Iterator it(headers); it.GetNext();) {
    fetcher->AddExtraRequestHeader(it.name() + ":" + it.value());
  }

  if (is_experimental) {
    fetcher->SetLoadFlags(net::LOAD_DO_NOT_SEND_COOKIES |
                          net::LOAD_DO_NOT_SAVE_COOKIES);
  } else {
    fetcher->SetLoadFlags(net::LOAD_DO_NOT_SAVE_COOKIES);
  }

  return fetcher;
}

void ContextualSuggestionsService::AccessTokenAvailable(
    std::unique_ptr<net::URLFetcher> fetcher,
    ContextualSuggestionsCallback callback,
    const GoogleServiceAuthError& error,
    const std::string& access_token) {
  DCHECK(token_fetcher_);
  token_fetcher_.reset();  // Delete token_fetcher_.

  if (error.state() != GoogleServiceAuthError::NONE) {
    std::move(callback).Run(nullptr);
    return;
  }
  DCHECK(!access_token.empty());
  fetcher->AddExtraRequestHeader(
      base::StringPrintf("Authorization: Bearer %s", access_token.c_str()));
  std::move(callback).Run(std::move(fetcher));
}
