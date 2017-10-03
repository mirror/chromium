// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web_view/internal/metrics/web_view_metrics_service_client.h"

#include "components/metrics/metrics_service.h"
#include "components/metrics/net/cellular_logic_helper.h"
#include "components/metrics/net/net_metrics_log_uploader.h"
#include "components/metrics/version_utils.h"
#include "ios/web_view/internal/app/application_context.h"

metrics::MetricsService* WebViewMetricsServiceClient::GetMetricsService() {
  return metrics_service_.get();
}

void WebViewMetricsServiceClient::SetMetricsClientId(
    const std::string& client_id) {}

int32_t WebViewMetricsServiceClient::GetProduct() {
  return metrics::ChromeUserMetricsExtension::IOS_WEBVIEW;
}

std::string WebViewMetricsServiceClient::GetApplicationLocale() {
  return ios_web_view::ApplicationContext::GetInstance()
      ->GetApplicationLocale();
}

bool WebViewMetricsServiceClient::GetBrand(std::string* brand_code) {
  return false;
}

metrics::SystemProfileProto::Channel WebViewMetricsServiceClient::GetChannel() {
  return metrics::SystemProfileProto::CHANNEL_UNKNOWN;
}

std::string WebViewMetricsServiceClient::GetVersionString() {
  return metrics::GetVersionString();
}

void WebViewMetricsServiceClient::CollectFinalMetricsForLog(
    const base::Closure& done_callback) {
  done_callback.Run();
}

std::unique_ptr<metrics::MetricsLogUploader>
WebViewMetricsServiceClient::CreateUploader(
    base::StringPiece server_url,
    base::StringPiece mime_type,
    metrics::MetricsLogUploader::MetricServiceType service_type,
    const metrics::MetricsLogUploader::UploadCallback& on_upload_complete) {
  return base::MakeUnique<metrics::NetMetricsLogUploader>(
      ios_web_view::ApplicationContext::GetInstance()
          ->GetSystemURLRequestContext(),
      server_url, mime_type, service_type, on_upload_complete);
}

base::TimeDelta WebViewMetricsServiceClient::GetStandardUploadInterval() {
  return metrics::GetUploadInterval();
}
