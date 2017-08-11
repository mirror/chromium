// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FEEDBACK_FEEDBACK_UPLOADER_CHROME_H_
#define COMPONENTS_FEEDBACK_FEEDBACK_UPLOADER_CHROME_H_

#include "base/macros.h"
#include "components/feedback/feedback_uploader.h"
#include "components/keyed_service/core/keyed_service.h"
#include "google_apis/gaia/oauth2_token_service.h"

namespace content {
class BrowserContext;
}

namespace feedback {

class FeedbackUploaderChrome : public OAuth2TokenService::Consumer,
                               public FeedbackUploader,
                               public KeyedService {
 public:
  FeedbackUploaderChrome(
      content::BrowserContext* context,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);
  ~FeedbackUploaderChrome() override;

 private:
  // OAuth2TokenService::Consumer:
  void OnGetTokenSuccess(const OAuth2TokenService::Request* request,
                         const std::string& access_token,
                         const base::Time& expiration_time) override;
  void OnGetTokenFailure(const OAuth2TokenService::Request* request,
                         const GoogleServiceAuthError& error) override;

  // feedback::FeedbackUploader:
  void DispatchReport(scoped_refptr<FeedbackReport> report) override;

  void SendReport(const std::string& access_token);

  // Browser context this uploader was created for.
  content::BrowserContext* context_;

  scoped_refptr<FeedbackReport> report_;

  std::unique_ptr<OAuth2TokenService::Request> access_token_request_;

  DISALLOW_COPY_AND_ASSIGN(FeedbackUploaderChrome);
};

}  // namespace feedback

#endif  // COMPONENTS_FEEDBACK_FEEDBACK_UPLOADER_CHROME_H_
