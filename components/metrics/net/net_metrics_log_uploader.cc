// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics/net/net_metrics_log_uploader.h"

#include "base/base64.h"
#include "base/command_line.h"
#include "base/metrics/histogram_macros.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "components/encrypted_messages/encrypted_message.pb.h"
#include "components/encrypted_messages/message_encrypter.h"
#include "components/metrics/metrics_log_uploader.h"
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"
#include "third_party/zlib/google/compression_utils.h"
#include "url/gurl.h"

namespace {

// Constants used for encrypting logs that are sent over HTTP. The
// corresponding private key is used by the metrics server to decrypt logs.
char kEncryptedMessageLabel[] = "metrics log";

static const uint8_t kServerPublicKey[] = {
    0x51, 0xcc, 0x52, 0x67, 0x42, 0x47, 0x3b, 0x10, 0xe8, 0x63, 0x18,
    0x3c, 0x61, 0xa7, 0x96, 0x76, 0x86, 0x91, 0x40, 0x71, 0x39, 0x5f,
    0x31, 0x1a, 0x39, 0x5b, 0x76, 0xb1, 0x6b, 0x3d, 0x6a, 0x2b};

static const uint32_t kServerPublicKeyVersion = 1;

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotation(
    const metrics::MetricsLogUploader::MetricServiceType& service_type) {
  // The code in this function should remain so that we won't need a default
  // case that does not have meaningful annotation.
  if (service_type == metrics::MetricsLogUploader::UMA) {
    return net::DefineNetworkTrafficAnnotation("metrics_report_uma", R"(
        semantics {
          sender: "Metrics UMA Log Uploader"
          description:
            "Report of usage statistics and crash-related data about Chromium. "
            "Usage statistics contain information such as preferences, button "
            "clicks, and memory usage and do not include web page URLs or "
            "personal information. See more at "
            "https://www.google.com/chrome/browser/privacy/ under 'Usage "
            "statistics and crash reports'. Usage statistics are tied to a "
            "pseudonymous machine identifier and not to your email address."
          trigger:
            "Reports are automatically generated on startup and at intervals "
            "while Chromium is running."
          data:
            "A protocol buffer with usage statistics and crash related data."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "Users can enable or disable this feature by disabling "
            "'Automatically send usage statistics and crash reports to Google' "
            "in Chromium's settings under Advanced Settings, Privacy. The "
            "feature is enabled by default."
          chrome_policy {
            MetricsReportingEnabled {
              policy_options {mode: MANDATORY}
              MetricsReportingEnabled: false
            }
          }
        })");
  } else {
    DCHECK_EQ(service_type, metrics::MetricsLogUploader::UKM);
    return net::DefineNetworkTrafficAnnotation("metrics_report_ukm", R"(
        semantics {
          sender: "Metrics UKM Log Uploader"
          description:
            "Report of usage statistics that are keyed by URLs to Chromium, "
            "sent only if the profile has History Sync. This includes "
            "information about the web pages you visit and your usage of them, "
            "such as page load speed. This will also include URLs and "
            "statistics related to downloaded files. If Extension Sync is "
            "enabled, these statistics will also include information about "
            "the extensions that have been installed from Chrome Web Store. "
            "Google only stores usage statistics associated with published "
            "extensions, and URLs that are known by Google’s search index. "
            "Usage statistics are tied to a pseudonymous machine identifier "
            "and not to your email address."
          trigger:
            "Reports are automatically generated on startup and at intervals "
            "while Chromium is running with Sync enabled."
          data:
            "A protocol buffer with usage statistics and associated URLs."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "Users can enable or disable this feature by disabling "
            "'Automatically send usage statistics and crash reports to Google' "
            "in Chromium's settings under Advanced Settings, Privacy. This is "
            "only enabled if all active profiles have History/Extension Sync "
            "enabled without a Sync passphrase."
          chrome_policy {
            MetricsReportingEnabled {
              policy_options {mode: MANDATORY}
              MetricsReportingEnabled: false
            }
          }
        })");
  }
}

}  // namespace

namespace metrics {

NetMetricsLogUploader::NetMetricsLogUploader(
    net::URLRequestContextGetter* request_context_getter,
    base::StringPiece server_url,
    base::StringPiece mime_type,
    MetricsLogUploader::MetricServiceType service_type,
    const MetricsLogUploader::UploadCallback& on_upload_complete)
    : request_context_getter_(request_context_getter),
      server_url_(server_url),
      insecure_server_url_(""),
      mime_type_(mime_type.data(), mime_type.size()),
      service_type_(service_type),
      on_upload_complete_(on_upload_complete) {}

NetMetricsLogUploader::NetMetricsLogUploader(
    net::URLRequestContextGetter* request_context_getter,
    base::StringPiece server_url,
    base::StringPiece insecure_server_url,
    base::StringPiece mime_type,
    MetricsLogUploader::MetricServiceType service_type,
    const MetricsLogUploader::UploadCallback& on_upload_complete)
    : request_context_getter_(request_context_getter),
      server_url_(server_url),
      insecure_server_url_(insecure_server_url),
      mime_type_(mime_type.data(), mime_type.size()),
      service_type_(service_type),
      on_upload_complete_(on_upload_complete) {}

NetMetricsLogUploader::~NetMetricsLogUploader() {
}

void NetMetricsLogUploader::UploadLog(const std::string& compressed_log_data,
                                      const std::string& log_hash) {
  UploadLogToURL(compressed_log_data, log_hash, GURL(server_url_));
}

void NetMetricsLogUploader::UploadLogToURL(
    const std::string& compressed_log_data,
    const std::string& log_hash,
    const GURL& url) {
  last_attempted_log_ = compressed_log_data;
  last_attempted_hash_ = log_hash;
  DCHECK(!log_hash.empty());
  current_fetch_ =
      net::URLFetcher::Create(url, net::URLFetcher::POST, this,
                              GetNetworkTrafficAnnotation(service_type_));
  auto service = data_use_measurement::DataUseUserData::UMA;

  switch (service_type_) {
    case MetricsLogUploader::UMA:
      service = data_use_measurement::DataUseUserData::UMA;
      break;
    case MetricsLogUploader::UKM:
      service = data_use_measurement::DataUseUserData::UKM;
      break;
  }
  data_use_measurement::DataUseUserData::AttachToFetcher(current_fetch_.get(),
                                                         service);
  current_fetch_->SetRequestContext(request_context_getter_);
  // If we are not using HTTPS for this upload, encrypt it.
  if (!url.SchemeIs(url::kHttpsScheme)) {
    std::string uncompressed_log;
    // Since we are encrypting the log, we will uncompress it first, then
    // compress it again once it's encrypted.
    compression::GzipUncompress(compressed_log_data, &uncompressed_log);
    encrypted_messages::EncryptedMessage encrypted_message;
    encrypted_messages::EncryptedMessage encrypted_hash;
    encrypted_messages::EncryptSerializedMessage(
        kServerPublicKey, kServerPublicKeyVersion, kEncryptedMessageLabel,
        uncompressed_log, &encrypted_message);
    encrypted_messages::EncryptSerializedMessage(
        kServerPublicKey, kServerPublicKeyVersion, kEncryptedMessageLabel,
        log_hash, &encrypted_hash);
    std::string serialized_encrypted_message;
    std::string serialized_encrypted_hash;
    encrypted_message.SerializeToString(&serialized_encrypted_message);
    encrypted_hash.SerializeToString(&serialized_encrypted_hash);
    std::string base64_encoded_hash;
    base::Base64Encode(serialized_encrypted_hash, &base64_encoded_hash);
    current_fetch_->AddExtraRequestHeader("X-Chrome-UMA-Log-SHA1: " +
                                          base64_encoded_hash);
    std::string compressed_encrypted_log;
    compression::GzipCompress(serialized_encrypted_message,
                              &compressed_encrypted_log);
    current_fetch_->SetUploadData(mime_type_, compressed_encrypted_log);
  } else {
    current_fetch_->AddExtraRequestHeader("X-Chrome-UMA-Log-SHA1: " + log_hash);
    current_fetch_->SetUploadData(mime_type_, compressed_log_data);
  }
  // Tell the server that we're uploading gzipped protobufs.
  current_fetch_->AddExtraRequestHeader("content-encoding: gzip");

  // Drop cookies and auth data.
  current_fetch_->SetLoadFlags(net::LOAD_DO_NOT_SAVE_COOKIES |
                               net::LOAD_DO_NOT_SEND_AUTH_DATA |
                               net::LOAD_DO_NOT_SEND_COOKIES);
  current_fetch_->Start();
}

void NetMetricsLogUploader::OnURLFetchComplete(const net::URLFetcher* source) {
  // We're not allowed to re-use the existing |URLFetcher|s, so free them here.
  // Note however that |source| is aliased to the fetcher, so we should be
  // careful not to delete it too early.
  DCHECK_EQ(current_fetch_.get(), source);
  int response_code = source->GetResponseCode();
  if (response_code == net::URLFetcher::RESPONSE_CODE_INVALID)
    response_code = -1;
  int error_code = 0;
  const net::URLRequestStatus& request_status = source->GetStatus();
  if (request_status.status() != net::URLRequestStatus::SUCCESS) {
    error_code = request_status.error();
  }
  bool was_https = source->GetURL().SchemeIs(url::kHttpsScheme);
  current_fetch_.reset();
  // TODO(carlosil): Once LogResponseOrErrorCode is public and distinguishes
  // between HTTPS and HTTP, it should be called from here instead of from
  // the callback (So we can log cases when we are about to retry, which won't
  // call the callback).
  // If the last attempt was over HTTPS, we couldn't reach the server and the
  // insecure URL is set, we immediately retry the connection over HTTP.
  // Currently we only retry if the retry-uma-over-http flag is set.
  if (!insecure_server_url_.is_empty() && was_https && error_code != 0 &&
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          "retry-uma-over-http")) {
    UploadLogToURL(last_attempted_log_, last_attempted_hash_,
                   insecure_server_url_);
  } else {  // Only call on_upload_complete if we are not about to retry.
    on_upload_complete_.Run(response_code, error_code);
  }
}

}  // namespace metrics
