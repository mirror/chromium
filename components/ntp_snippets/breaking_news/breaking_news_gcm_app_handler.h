// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_BREAKING_NEWS_BREAKING_NEWS_GCM_APP_HANDLER_H_
#define COMPONENTS_NTP_SNIPPETS_BREAKING_NEWS_BREAKING_NEWS_GCM_APP_HANDLER_H_

#include "base/memory/weak_ptr.h"
#include "base/time/clock.h"
#include "components/gcm_driver/gcm_app_handler.h"
#include "components/gcm_driver/instance_id/instance_id.h"
#include "components/ntp_snippets/breaking_news/breaking_news_listener.h"
#include "components/ntp_snippets/breaking_news/subscription_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

class PrefRegistrySimple;

namespace gcm {
class GCMDriver;
}

namespace instance_id {
class InstanceIDDriver;
}

namespace ntp_snippets {

// Handler for pushed GCM breaking news. It retrieves a subscription token
// from the GCM server and registers/unregisters itself with the GCM service to
// be called upon received push breaking news.
class BreakingNewsGCMAppHandler : public BreakingNewsListener,
                                  public gcm::GCMAppHandler {
 public:
  // Callbacks for JSON parsing to allow injecting platform-dependent code.
  using SuccessCallback =
      base::Callback<void(std::unique_ptr<base::Value> result)>;
  using ErrorCallback = base::Callback<void(const std::string& error)>;
  using ParseJSONCallback =
      base::Callback<void(const std::string& raw_json_string,
                          const SuccessCallback& success_callback,
                          const ErrorCallback& error_callback)>;

  using OnNewContentCallback =
      base::Callback<void(std::unique_ptr<base::Value> content)>;

  BreakingNewsGCMAppHandler(
      gcm::GCMDriver* gcm_driver,
      instance_id::InstanceIDDriver* instance_id_driver,
      PrefService* pref_service_,
      std::unique_ptr<SubscriptionManager> subscription_manager,
      const ParseJSONCallback& parse_json_callback,
      std::unique_ptr<base::Clock> clock);

  // If still listening, calls StopListening()
  ~BreakingNewsGCMAppHandler() override;

  // BreakingNewsListener overrides.
  void StartListening(OnNewContentCallback on_new_content_callback) override;
  void StopListening() override;

  // GCMAppHandler overrides.
  void ShutdownHandler() override;
  void OnStoreReset() override;
  void OnMessage(const std::string& app_id,
                 const gcm::IncomingMessage& message) override;
  void OnMessagesDeleted(const std::string& app_id) override;
  void OnSendError(const std::string& app_id,
                   const gcm::GCMClient::SendErrorDetails& details) override;
  void OnSendAcknowledged(const std::string& app_id,
                          const std::string& message_id) override;

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  // If there is no subscription token or |force_token_retrieval|, retrieves a
  // GCM subscription token and subscribes to our server with it. Otherwise,
  // subscribes to our server with the existing token.
  void Subscribe(bool force_token_retrieval);

  // Called when subscription token is obtained from the GCM.
  void OnTokenRetrieval(const std::string& subscription_token,
                        instance_id::InstanceID::Result result);

  // Called periodically to validate the subscription token (it is stored on
  // disk and may become corrupted) and resubscribe with the new token if the
  // old one is invalid.
  void ResubscribeIfInvalidToken();

  // Called when token validation response is obtained from the GCM.
  void OnTokenValidation(bool is_valid);

  void ScheduleNextTokenValidationIfNeeded();

  // Called after successfully parsing the received suggestion JSON.
  void OnJsonSuccess(std::unique_ptr<base::Value> content);

  // Called in case the received suggestion JSON inside the GCM has a parse
  // error.
  void OnJsonError(const std::string& json_str, const std::string& error);

  gcm::GCMDriver* const gcm_driver_;
  instance_id::InstanceIDDriver* const instance_id_driver_;
  PrefService* const pref_service_;
  const std::unique_ptr<SubscriptionManager> subscription_manager_;
  const ParseJSONCallback parse_json_callback_;
  std::unique_ptr<base::Clock> clock_;

  // Called after every time a new message is received in OnMessage() to notify
  // the content provider.
  OnNewContentCallback on_new_content_callback_;

  base::WeakPtrFactory<BreakingNewsGCMAppHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BreakingNewsGCMAppHandler);
};
}  // namespace ntp_snippets

#endif  // COMPONENTS_NTP_SNIPPETS_BREAKING_NEWS_BREAKING_NEWS_GCM_APP_HANDLER_H_
