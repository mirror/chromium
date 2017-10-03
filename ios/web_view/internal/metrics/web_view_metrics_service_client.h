// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_METRICS_WEB_VIEW_METRICS_SERVICE_CLIENT_H
#define IOS_WEB_VIEW_INTERNAL_METRICS_WEB_VIEW_METRICS_SERVICE_CLIENT_H

#include "components/metrics/metrics_service_client.h"

// WebViewMetricsServiceClient provides an implementation of
// MetricsServiceClient that depends on //ios/web_view/.
class WebViewMetricsServiceClient : public metrics::MetricsServiceClient {
 public:
  ~WebViewMetricsServiceClient() override = default;

  // metrics::MetricsServiceClient implementation.
  metrics::MetricsService* GetMetricsService() override;
  void SetMetricsClientId(const std::string& client_id) override;
  int32_t GetProduct() override;
  std::string GetApplicationLocale() override;
  bool GetBrand(std::string* brand_code) override;
  metrics::SystemProfileProto::Channel GetChannel() override;
  std::string GetVersionString() override;
  void CollectFinalMetricsForLog(const base::Closure& done_callback) override;
  std::unique_ptr<metrics::MetricsLogUploader> CreateUploader(
      base::StringPiece server_url,
      base::StringPiece mime_type,
      metrics::MetricsLogUploader::MetricServiceType service_type,
      const metrics::MetricsLogUploader::UploadCallback& on_upload_complete)
      override;
  base::TimeDelta GetStandardUploadInterval() override;

 private:
  // The MetricsService that |this| is a client of.
  std::unique_ptr<metrics::MetricsService> metrics_service_;

  DISALLOW_COPY_AND_ASSIGN(WebViewMetricsServiceClient);
};

#endif  // IOS_WEB_VIEW_INTERNAL_METRICS_WEB_VIEW_METRICS_SERVICE_CLIENT_H
