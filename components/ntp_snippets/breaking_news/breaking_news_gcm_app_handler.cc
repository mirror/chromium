// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/breaking_news/breaking_news_gcm_app_handler.h"

#include "base/strings/string_util.h"
#include "base/task_scheduler/post_task.h"
#include "components/gcm_driver/gcm_driver.h"
#include "components/gcm_driver/gcm_profile_service.h"
#include "components/gcm_driver/instance_id/instance_id.h"
#include "components/gcm_driver/instance_id/instance_id_driver.h"
#include "components/ntp_snippets/pref_names.h"

using instance_id::InstanceID;

namespace ntp_snippets {

const char kBreakingNewsGCMAppID[] = "com.google.breakingnews.gcm";

// The sender ID is used in the registration process.
// See: https://developers.google.com/cloud-messaging/gcm#senderid
const char kBreakingNewsGCMSenderId[] = "667617379155";

// OAuth2 Scope passed to getToken to obtain GCM registration tokens.
// Must match Java GoogleCloudMessaging.INSTANCE_ID_SCOPE.
const char kGCMScope[] = "GCM";

// Key of the news json in the data in the pushed breaking news.
const char kPushedNewsKey[] = "payload";

const base::TimeDelta kTokenValidationPeriod = base::TimeDelta::FromHours(24);

BreakingNewsGCMAppHandler::BreakingNewsGCMAppHandler(
    gcm::GCMDriver* gcm_driver,
    instance_id::InstanceIDDriver* instance_id_driver,
    PrefService* pref_service,
    std::unique_ptr<SubscriptionManager> subscription_manager,
    const ParseJSONCallback& parse_json_callback,
    std::unique_ptr<base::Clock> clock)
    : gcm_driver_(gcm_driver),
      instance_id_driver_(instance_id_driver),
      pref_service_(pref_service),
      subscription_manager_(std::move(subscription_manager)),
      parse_json_callback_(parse_json_callback),
      clock_(std::move(clock)),
      weak_ptr_factory_(this) {}

BreakingNewsGCMAppHandler::~BreakingNewsGCMAppHandler() {
  StopListening();
}

void BreakingNewsGCMAppHandler::StartListening(
    OnNewContentCallback on_new_content_callback) {
#if !defined(OS_ANDROID)
  NOTREACHED()
      << "The BreakingNewsGCMAppHandler should only be used on Android.";
#endif
  Subscribe(/*force_token_retrieval=*/false);
  on_new_content_callback_ = std::move(on_new_content_callback);
  gcm_driver_->AddAppHandler(kBreakingNewsGCMAppID, this);
  ScheduleNextTokenValidationIfNeeded();
}

void BreakingNewsGCMAppHandler::StopListening() {
  DCHECK_EQ(gcm_driver_->GetAppHandler(kBreakingNewsGCMAppID), this);
  gcm_driver_->RemoveAppHandler(kBreakingNewsGCMAppID);
  on_new_content_callback_ = OnNewContentCallback();
  subscription_manager_->Unsubscribe();
}

void BreakingNewsGCMAppHandler::Subscribe(bool force_token_retrieval) {
  // TODO(mamir): This logic should be moved to the SubscriptionManager.
  std::string token =
      pref_service_->GetString(prefs::kBreakingNewsGCMSubscriptionTokenCache);
  // If a token has been already obtained, subscribe directly at the content
  // suggestions server. Otherwise, obtain a GCM token first.
  if (!token.empty() && !force_token_retrieval) {
    if (!subscription_manager_->IsSubscribed() ||
        subscription_manager_->NeedsToResubscribe()) {
      subscription_manager_->Subscribe(token);
    }
    return;
  }

  instance_id_driver_->GetInstanceID(kBreakingNewsGCMAppID)
      ->GetToken(kBreakingNewsGCMSenderId, kGCMScope,
                 /*options=*/std::map<std::string, std::string>(),
                 base::Bind(&BreakingNewsGCMAppHandler::OnTokenRetrieval,
                            weak_ptr_factory_.GetWeakPtr()));
}

void BreakingNewsGCMAppHandler::OnTokenRetrieval(
    const std::string& subscription_token,
    InstanceID::Result result) {
  switch (result) {
    case InstanceID::SUCCESS:
      pref_service_->SetString(prefs::kBreakingNewsGCMSubscriptionTokenCache,
                               subscription_token);
      subscription_manager_->Subscribe(subscription_token);
      return;
    case InstanceID::INVALID_PARAMETER:
    case InstanceID::DISABLED:
    case InstanceID::ASYNC_OPERATION_PENDING:
    case InstanceID::SERVER_ERROR:
    case InstanceID::UNKNOWN_ERROR:
      DLOG(WARNING)
          << "Push messaging subscription failed; InstanceID::Result = "
          << result;
      break;
    case InstanceID::NETWORK_ERROR:
      break;
  }
}

void BreakingNewsGCMAppHandler::ResubscribeIfInvalidToken() {
  std::string token =
      pref_service_->GetString(prefs::kBreakingNewsGCMSubscriptionTokenCache);
  instance_id_driver_->GetInstanceID(kBreakingNewsGCMAppID)
      ->ValidateToken(kBreakingNewsGCMSenderId, kGCMScope, token,
                      base::Bind(&BreakingNewsGCMAppHandler::OnTokenValidation,
                                 weak_ptr_factory_.GetWeakPtr()));
}

void BreakingNewsGCMAppHandler::OnTokenValidation(bool is_valid) {
  // TODO(crbug.com/744557): Record time from last validation.
  ScheduleNextTokenValidationIfNeeded();
  if (!is_valid) {
    // TODO(crbug.com/744557): Record time since last succesful validation.
    Subscribe(/*force_token_retrieval=*/true);
  }
}

void BreakingNewsGCMAppHandler::ScheduleNextTokenValidationIfNeeded() {
  base::Time nextValidationTime =
      base::Time::FromInternalValue(pref_service_->GetInt64(
          prefs::kBreakingNewsGCMNextScheduledTokenValidationTime));

  if (nextValidationTime < clock_->Now()) {
    nextValidationTime = clock_->Now() + kTokenValidationPeriod;
    pref_service_->SetInt64(
        prefs::kBreakingNewsGCMNextScheduledTokenValidationTime,
        nextValidationTime.ToInternalValue());
    base::PostDelayedTaskWithTraits(
        FROM_HERE, {base::TaskPriority::BACKGROUND},
        base::Bind(&BreakingNewsGCMAppHandler::ResubscribeIfInvalidToken,
                   weak_ptr_factory_.GetWeakPtr()),
        /*delay=*/kTokenValidationPeriod);
  }
}

void BreakingNewsGCMAppHandler::ShutdownHandler() {}

void BreakingNewsGCMAppHandler::OnStoreReset() {
  pref_service_->ClearPref(prefs::kBreakingNewsGCMSubscriptionTokenCache);
}

void BreakingNewsGCMAppHandler::OnMessage(const std::string& app_id,
                                          const gcm::IncomingMessage& message) {
  DCHECK_EQ(app_id, kBreakingNewsGCMAppID);

  gcm::MessageData::const_iterator it = message.data.find(kPushedNewsKey);
  if (it == message.data.end()) {
    LOG(WARNING)
        << "Receiving pushed content failure: Breaking News ID missing.";
    return;
  }

  std::string news = it->second;

  parse_json_callback_.Run(news,
                           base::Bind(&BreakingNewsGCMAppHandler::OnJsonSuccess,
                                      weak_ptr_factory_.GetWeakPtr()),
                           base::Bind(&BreakingNewsGCMAppHandler::OnJsonError,
                                      weak_ptr_factory_.GetWeakPtr(), news));
}

void BreakingNewsGCMAppHandler::OnMessagesDeleted(const std::string& app_id) {
  // Messages don't get deleted.
  NOTREACHED() << "BreakingNewsGCMAppHandler messages don't get deleted.";
}

void BreakingNewsGCMAppHandler::OnSendError(
    const std::string& app_id,
    const gcm::GCMClient::SendErrorDetails& details) {
  // Should never be called because we don't send GCM messages to
  // the server.
  NOTREACHED() << "BreakingNewsGCMAppHandler doesn't send GCM messages.";
}

void BreakingNewsGCMAppHandler::OnSendAcknowledged(
    const std::string& app_id,
    const std::string& message_id) {
  // Should never be called because we don't send GCM messages to
  // the server.
  NOTREACHED() << "BreakingNewsGCMAppHandler doesn't send GCM messages.";
}

void BreakingNewsGCMAppHandler::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterStringPref(prefs::kBreakingNewsGCMSubscriptionTokenCache,
                               /*default_value=*/std::string());
  registry->RegisterInt64Pref(
      prefs::kBreakingNewsGCMNextScheduledTokenValidationTime,
      /*default_value=*/0);
}

void BreakingNewsGCMAppHandler::OnJsonSuccess(
    std::unique_ptr<base::Value> content) {
  on_new_content_callback_.Run(std::move(content));
}

void BreakingNewsGCMAppHandler::OnJsonError(const std::string& json_str,
                                            const std::string& error) {
  LOG(WARNING) << "Error parsing JSON:" << error
               << " when parsing:" << json_str;
}

}  // namespace ntp_snippets
