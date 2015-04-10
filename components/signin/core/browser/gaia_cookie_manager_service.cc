// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/browser/gaia_cookie_manager_service.h"

#include <queue>
#include <vector>

#include "base/json/json_reader.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/signin/core/browser/signin_metrics.h"
#include "google_apis/gaia/gaia_auth_fetcher.h"
#include "google_apis/gaia/gaia_constants.h"
#include "google_apis/gaia/gaia_urls.h"
#include "google_apis/gaia/oauth2_token_service.h"
#include "net/base/load_flags.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace {

// In case of an error while fetching using the GaiaAuthFetcher, retry with
// exponential backoff. Try up to 7 times within 15 minutes.
const net::BackoffEntry::Policy kBackoffPolicy = {
  // Number of initial errors (in sequence) to ignore before applying
  // exponential back-off rules.
  0,

  // Initial delay for exponential backoff in ms.
  1000,

  // Factor by which the waiting time will be multiplied.
  3,

  // Fuzzing percentage. ex: 10% will spread requests randomly
  // between 90%-100% of the calculated time.
  0.2, // 20%

  // Maximum amount of time we are willing to delay our request in ms.
  1000 * 60 * 60 * 4, // 15 minutes.

  // Time to keep an entry from being discarded even when it
  // has no significant state, -1 to never discard.
  -1,

  // Don't use initial delay unless the last request was an error.
  false,
};

const int kMaxGaiaAuthFetcherRetries = 8;

bool IsTransientError(const GoogleServiceAuthError& error) {
  return error.state() == GoogleServiceAuthError::CONNECTION_FAILED ||
      error.state() == GoogleServiceAuthError::SERVICE_UNAVAILABLE ||
      error.state() == GoogleServiceAuthError::REQUEST_CANCELED;
}

enum GaiaCookieRequestType {
  ADD_ACCOUNT,
  LOG_OUT_ALL_ACCOUNTS,
  LOG_OUT_ONE_ACCOUNT,
  LIST_ACCOUNTS
};

}  // namespace

GaiaCookieManagerService::GaiaCookieRequest::GaiaCookieRequest(
    GaiaCookieRequestType request_type,
    const std::string& account_id,
    const GaiaCookieManagerService::ListAccountsCallback&
        list_accounts_callback)
  : request_type_(request_type),
    account_id_(account_id),
    list_accounts_callback_(list_accounts_callback) {}

GaiaCookieManagerService::GaiaCookieRequest::~GaiaCookieRequest() {
}

// static
GaiaCookieManagerService::GaiaCookieRequest
GaiaCookieManagerService::GaiaCookieRequest::CreateAddAccountRequest(
      const std::string& account_id) {
  return GaiaCookieManagerService::GaiaCookieRequest(
      GaiaCookieManagerService::GaiaCookieRequestType::ADD_ACCOUNT,
      account_id,
      GaiaCookieManagerService::ListAccountsCallback());
}

// static
GaiaCookieManagerService::GaiaCookieRequest
GaiaCookieManagerService::GaiaCookieRequest::CreateLogOutRequest() {
  return GaiaCookieManagerService::GaiaCookieRequest(
      GaiaCookieManagerService::GaiaCookieRequestType::LOG_OUT,
      std::string(),
      GaiaCookieManagerService::ListAccountsCallback());
}

GaiaCookieManagerService::GaiaCookieRequest
GaiaCookieManagerService::GaiaCookieRequest::CreateListAccountsRequest(
    const GaiaCookieManagerService::ListAccountsCallback&
        list_accounts_callback) {
  return GaiaCookieManagerService::GaiaCookieRequest(
      GaiaCookieManagerService::GaiaCookieRequestType::LIST_ACCOUNTS,
      std::string(),
      list_accounts_callback);
}

GaiaCookieManagerService::ExternalCcResultFetcher::ExternalCcResultFetcher(
    GaiaCookieManagerService* helper)
    : helper_(helper) {
  DCHECK(helper_);
}

GaiaCookieManagerService::ExternalCcResultFetcher::~ExternalCcResultFetcher() {
  CleanupTransientState();
}

std::string
GaiaCookieManagerService::ExternalCcResultFetcher::GetExternalCcResult() {
  std::vector<std::string> results;
  for (ResultMap::const_iterator it = results_.begin(); it != results_.end();
       ++it) {
    results.push_back(it->first + ":" + it->second);
  }
  return JoinString(results, ",");
}

void GaiaCookieManagerService::ExternalCcResultFetcher::Start() {
  m_external_cc_result_start_time_ = base::Time::Now();

  CleanupTransientState();
  results_.clear();
  helper_->gaia_auth_fetcher_.reset(
      new GaiaAuthFetcher(this, helper_->source_, helper_->request_context()));
  helper_->gaia_auth_fetcher_->StartGetCheckConnectionInfo();

  // Some fetches may timeout.  Start a timer to decide when the result fetcher
  // has waited long enough.
  // TODO(rogerta): I have no idea how long to wait before timing out.
  // Gaia folks say this should take no more than 2 second even in mobile.
  // This will need to be tweaked.
  timer_.Start(FROM_HERE, base::TimeDelta::FromSeconds(5), this,
               &GaiaCookieManagerService::ExternalCcResultFetcher::Timeout);
}

bool GaiaCookieManagerService::ExternalCcResultFetcher::IsRunning() {
  return helper_->gaia_auth_fetcher_ || fetchers_.size() > 0u;
}

void GaiaCookieManagerService::ExternalCcResultFetcher::TimeoutForTests() {
  Timeout();
}

void GaiaCookieManagerService::ExternalCcResultFetcher::
    OnGetCheckConnectionInfoSuccess(const std::string& data) {
  helper_->gaia_auth_fetcher_backoff_.InformOfRequest(true);
  gaia_auth_fetcher_timer_.Stop();
  scoped_ptr<base::Value> value(base::JSONReader::Read(data));
  const base::ListValue* list;
  if (!value || !value->GetAsList(&list)) {
    CleanupTransientState();
    GetCheckConnectionInfoCompleted(false);
    return;
  }

  // If there is nothing to check, terminate immediately.
  if (list->GetSize() == 0) {
    CleanupTransientState();
    GetCheckConnectionInfoCompleted(true);
    return;
  }

  // Start a fetcher for each connection URL that needs to be checked.
  for (size_t i = 0; i < list->GetSize(); ++i) {
    const base::DictionaryValue* dict;
    if (list->GetDictionary(i, &dict)) {
      std::string token;
      std::string url;
      if (dict->GetString("carryBackToken", &token) &&
          dict->GetString("url", &url)) {
        results_[token] = "null";
        net::URLFetcher* fetcher = CreateFetcher(GURL(url));
        fetchers_[fetcher->GetOriginalURL()] = std::make_pair(token, fetcher);
        fetcher->Start();
      }
    }
  }
}

void GaiaCookieManagerService::ExternalCcResultFetcher::
    OnGetCheckConnectionInfoError(const GoogleServiceAuthError& error) {
  if (++helper_->gaia_auth_fetcher_retries_ < kMaxGaiaAuthFetcherRetries &&
      IsTransientError(error)) {
    helper_->gaia_auth_fetcher_backoff_.InformOfRequest(false);
    gaia_auth_fetcher_timer_.Start(
        FROM_HERE, helper_->gaia_auth_fetcher_backoff_.GetTimeUntilRelease(),
        this, &GaiaCookieManagerService::ExternalCcResultFetcher::Start);
    return;
  }

  CleanupTransientState();
  GetCheckConnectionInfoCompleted(false);
}

net::URLFetcher*
GaiaCookieManagerService::ExternalCcResultFetcher::CreateFetcher(
    const GURL& url) {
  net::URLFetcher* fetcher =
      net::URLFetcher::Create(0, url, net::URLFetcher::GET, this);
  fetcher->SetRequestContext(helper_->request_context());
  fetcher->SetLoadFlags(net::LOAD_DO_NOT_SEND_COOKIES |
                        net::LOAD_DO_NOT_SAVE_COOKIES);

  // Fetchers are sometimes cancelled because a network change was detected,
  // especially at startup and after sign-in on ChromeOS.
  fetcher->SetAutomaticallyRetryOnNetworkChanges(1);
  return fetcher;
}

void GaiaCookieManagerService::ExternalCcResultFetcher::OnURLFetchComplete(
    const net::URLFetcher* source) {
  const GURL& url = source->GetOriginalURL();
  const net::URLRequestStatus& status = source->GetStatus();
  int response_code = source->GetResponseCode();
  if (status.is_success() && response_code == net::HTTP_OK &&
      fetchers_.count(url) > 0) {
    std::string data;
    source->GetResponseAsString(&data);
    // Only up to the first 16 characters of the response are important to GAIA.
    // Truncate if needed to keep amount data sent back to GAIA down.
    if (data.size() > 16)
      data.resize(16);
    results_[fetchers_[url].first] = data;

    // Clean up tracking of this fetcher.  The rest will be cleaned up after
    // the timer expires in CleanupTransientState().
    DCHECK_EQ(source, fetchers_[url].second);
    fetchers_.erase(url);
    delete source;

    // If all expected responses have been received, cancel the timer and
    // report the result.
    if (fetchers_.empty()) {
      CleanupTransientState();
      GetCheckConnectionInfoCompleted(true);
    }
  }
}

void GaiaCookieManagerService::ExternalCcResultFetcher::Timeout() {
  CleanupTransientState();
  GetCheckConnectionInfoCompleted(false);
}

void GaiaCookieManagerService::ExternalCcResultFetcher::
    CleanupTransientState() {
  timer_.Stop();
  helper_->gaia_auth_fetcher_.reset();

  for (URLToTokenAndFetcher::const_iterator it = fetchers_.begin();
       it != fetchers_.end(); ++it) {
    delete it->second.second;
  }
  fetchers_.clear();
}

void GaiaCookieManagerService::ExternalCcResultFetcher::
    GetCheckConnectionInfoCompleted(bool succeeded) {
  base::TimeDelta time_to_check_connections =
      base::Time::Now() - m_external_cc_result_start_time_;
  signin_metrics::LogExternalCcResultFetches(succeeded,
                                             time_to_check_connections);

  helper_->external_cc_result_fetched_ = true;
  // Since the ExternalCCResultFetcher is only Started in place of calling
  // StartFetchingMergeSession, we can assume we need to call
  // StartFetchingMergeSession. If this assumption becomes invalid, a Callback
  // will need to be passed to Start() and Run() here.
  helper_->StartFetchingMergeSession();
}

GaiaCookieManagerService::GaiaCookieManagerService(
    OAuth2TokenService* token_service,
    const std::string& source,
    SigninClient* signin_client)
    : token_service_(token_service),
      signin_client_(signin_client),
      external_cc_result_fetcher_(this),
      gaia_auth_fetcher_backoff_(&kBackoffPolicy),
      gaia_auth_fetcher_retries_(0),
      source_(source),
      external_cc_result_fetched_(false) {
}

GaiaCookieManagerService::~GaiaCookieManagerService() {
  CancelAll();
  DCHECK(requests_.empty());
}

void GaiaCookieManagerService::AddAccountToCookie(
    const std::string& account_id) {
  DCHECK(!account_id.empty());
  VLOG(1) << "GaiaCookieManagerService::AddAccountToCookie: " << account_id;
  requests_.push_back(GaiaCookieRequest::CreateAddAccountRequest(account_id));
  if (requests_.size() == 1)
    StartFetchingUbertoken();
}

void GaiaCookieManagerService::ListAccounts(
    const ListAccountsCallback& callback) {
  // Not implemented yet.
  NOTREACHED();

  // TODO(mlerman): Once this service listens to all GAIA cookie changes, cache
  // the results of ListAccounts, and return them here if the GAIA cookie
  // hasn't changed since the last call.

  // If there's a GAIA call being executed, wait for it to complete. If it was
  // another /ListAccounts then we'll use the results it caches.
  if (gaia_auth_fetcher_)
    return;

  VLOG(1) << "GaiaCookieManagerService::ListAccounts";
  gaia_auth_fetcher_.reset(
      new GaiaAuthFetcher(this, source_,
                          signin_client_->GetURLRequestContext()));
  gaia_auth_fetcher_->StartListAccounts();
}

void GaiaCookieManagerService::LogOutAllAccounts() {
  VLOG(1) << "GaiaCookieManagerService::LogOutAllAccounts";

  bool log_out_queued = false;
  if (!requests_.empty()) {
    // Track requests to keep; all other unstarted requests will be removed.
    std::vector<GaiaCookieRequest> requests_to_keep;

    // Check all pending, non-executing requests.
    for (auto it = requests_.begin() + 1; it != requests_.end(); ++it) {
      if (it->request_type() == GaiaCookieRequestType::ADD_ACCOUNT) {
        // We have a pending log in request for an account followed by
        // a signout.
        GoogleServiceAuthError error(GoogleServiceAuthError::REQUEST_CANCELED);
        SignalComplete(it->account_id(), error);
      }

      // Keep all requests except for ADD_ACCOUNTS.
      if (it->request_type() != GaiaCookieRequestType::ADD_ACCOUNT)
        requests_to_keep.push_back(*it);

      // Verify a LOG_OUT isn't already queued.
      if (it->request_type() == GaiaCookieRequestType::LOG_OUT)
        log_out_queued = true;
    }

    // Verify a LOG_OUT isn't currently being processed.
    if (requests_.front().request_type() == GaiaCookieRequestType::LOG_OUT)
      log_out_queued = true;

    // Remove all but the executing request. Re-add all requests being kept.
    if (requests_.size() > 1) {
      requests_.erase(requests_.begin() + 1, requests_.end());
      requests_.insert(
          requests_.end(), requests_to_keep.begin(), requests_to_keep.end());
    }
  }

  if (!log_out_queued) {
    requests_.push_back(GaiaCookieRequest::CreateLogOutRequest());
    if (requests_.size() == 1)
      StartLogOutUrlFetch();
  }
}

void GaiaCookieManagerService::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void GaiaCookieManagerService::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

void GaiaCookieManagerService::CancelAll() {
  VLOG(1) << "GaiaCookieManagerService::CancelAll";
  gaia_auth_fetcher_.reset();
  uber_token_fetcher_.reset();
  requests_.clear();
  gaia_auth_fetcher_timer_.Stop();
}

void GaiaCookieManagerService::SignalComplete(
    const std::string& account_id,
    const GoogleServiceAuthError& error) {
  // Its possible for the observer to delete |this| object.  Don't access
  // access any members after this calling the observer.  This method should
  // be the last call in any other method.
  FOR_EACH_OBSERVER(Observer, observer_list_,
                    OnAddAccountToCookieCompleted(account_id, error));
}

void GaiaCookieManagerService::StartLogOutUrlFetch() {
  DCHECK(requests_.front().request_type() == GaiaCookieRequestType::LOG_OUT);
  VLOG(1) << "GaiaCookieManagerService::StartLogOutUrlFetch";
  GURL logout_url(GaiaUrls::GetInstance()->service_logout_url().Resolve(
      base::StringPrintf("?source=%s", source_.c_str())));
  net::URLFetcher* fetcher =
      net::URLFetcher::Create(logout_url, net::URLFetcher::GET, this);
  fetcher->SetRequestContext(signin_client_->GetURLRequestContext());
  fetcher->Start();
}

void GaiaCookieManagerService::OnUbertokenSuccess(
    const std::string& uber_token) {
  DCHECK(requests_.front().request_type() ==
      GaiaCookieRequestType::ADD_ACCOUNT);
  VLOG(1) << "GaiaCookieManagerService::OnUbertokenSuccess"
          << " account=" << requests_.front().account_id();
  gaia_auth_fetcher_retries_ = 0;
  uber_token_ = uber_token;

  if (!external_cc_result_fetched_ &&
      !external_cc_result_fetcher_.IsRunning()) {
    external_cc_result_fetcher_.Start();
    return;
  }

  StartFetchingMergeSession();
}

void GaiaCookieManagerService::OnUbertokenFailure(
    const GoogleServiceAuthError& error) {
  VLOG(1) << "Failed to retrieve ubertoken"
          << " account=" << requests_.front().account_id()
          << " error=" << error.ToString();
  const std::string account_id = requests_.front().account_id();
  HandleNextRequest();
  SignalComplete(account_id, error);
}

void GaiaCookieManagerService::OnMergeSessionSuccess(const std::string& data) {
  VLOG(1) << "MergeSession successful account="
          << requests_.front().account_id();
  const std::string account_id = requests_.front().account_id();
  HandleNextRequest();
  SignalComplete(account_id, GoogleServiceAuthError::AuthErrorNone());

  gaia_auth_fetcher_backoff_.InformOfRequest(true);
  uber_token_ = std::string();
}

void GaiaCookieManagerService::OnMergeSessionFailure(
    const GoogleServiceAuthError& error) {
  VLOG(1) << "Failed MergeSession"
          << " account=" << requests_.front().account_id()
          << " error=" << error.ToString();

  if (++gaia_auth_fetcher_retries_ < kMaxGaiaAuthFetcherRetries &&
      IsTransientError(error)) {
    gaia_auth_fetcher_backoff_.InformOfRequest(false);
    gaia_auth_fetcher_timer_.Start(
        FROM_HERE, gaia_auth_fetcher_backoff_.GetTimeUntilRelease(), this,
        &GaiaCookieManagerService::StartFetchingMergeSession);
    return;
  }

  uber_token_ = std::string();
  const std::string account_id = requests_.front().account_id();
  HandleNextRequest();
  SignalComplete(account_id, error);
}

void GaiaCookieManagerService::StartFetchingUbertoken() {
  VLOG(1) << "GaiaCookieManagerService::StartFetching account_id="
          << requests_.front().account_id();
  uber_token_fetcher_.reset(
      new UbertokenFetcher(token_service_, this, source_,
                           signin_client_->GetURLRequestContext()));
  uber_token_fetcher_->StartFetchingToken(requests_.front().account_id());
}

void GaiaCookieManagerService::StartFetchingMergeSession() {
  DCHECK(!uber_token_.empty());
  gaia_auth_fetcher_.reset(
      new GaiaAuthFetcher(this, source_,
                          signin_client_->GetURLRequestContext()));

  gaia_auth_fetcher_->StartMergeSession(uber_token_,
      external_cc_result_fetcher_.GetExternalCcResult());
}

void GaiaCookieManagerService::OnURLFetchComplete(
    const net::URLFetcher* source) {
  DCHECK(requests_.front().request_type() == GaiaCookieRequestType::LOG_OUT);
  VLOG(1) << "GaiaCookieManagerService::OnURLFetchComplete";
  HandleNextRequest();
}

void GaiaCookieManagerService::HandleNextRequest() {
  VLOG(1) << "GaiaCookieManagerService::HandleNextRequest";
  requests_.pop_front();
  gaia_auth_fetcher_.reset();
  if (requests_.empty()) {
    VLOG(1) << "GaiaCookieManagerService::HandleNextRequest: no more";
    uber_token_fetcher_.reset();
  } else {
    switch (requests_.front().request_type()) {
      case GaiaCookieRequestType::ADD_ACCOUNT:
        StartFetchingUbertoken();
        break;
      case GaiaCookieRequestType::LOG_OUT:
        StartLogOutUrlFetch();
        break;
      case GaiaCookieRequestType::LIST_ACCOUNTS:
        break;
    };
  }
}
