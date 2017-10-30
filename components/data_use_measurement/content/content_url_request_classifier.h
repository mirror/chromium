// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_USE_MEASUREMENT_CONTENT_CONTENT_URL_REQUEST_CLASSIFIER_H_
#define COMPONENTS_DATA_USE_MEASUREMENT_CONTENT_CONTENT_URL_REQUEST_CLASSIFIER_H_

#include "components/data_use_measurement/core/url_request_classifier.h"
#include "content/public/common/resource_type.h"

namespace net {
class URLRequest;
}

namespace data_use_measurement {

// Returns true if the URLRequest |request| is initiated by user traffic.
bool IsUserRequest(const net::URLRequest& request);

class ContentURLRequestClassifier : public URLRequestClassifier {
 public:
  ContentURLRequestClassifier();

 private:
  // Foreground and background states of App and the tab that are of interest.
  enum AppTabState {
    APPBACKGROUND,
    APPFOREGROUND_TABBACKGROUND,
    APPFOREGROUND_TABFOREGROUND,
    MAX
  };

  // UrlRequestClassifier:
  bool IsUserRequest(const net::URLRequest& request) const override;
  DataUseUserData::DataUseContentType GetContentType(
      const net::URLRequest& request,
      const net::HttpResponseHeaders& response_headers) const override;
  void RecordPageTransitionUMA(uint64_t page_transition,
                               int64_t received_bytes) const override;
  bool IsFavIconRequest(const net::URLRequest& request) const override;

  void RecordUserTrafficResourceTypeUMA(const net::URLRequest& request,
                                        bool is_downstream,
                                        bool is_app_visible,
                                        bool is_tab_visible,
                                        int64_t bytes) override;

  // User traffic data use by resource type is logged in 1KB increments. The
  // remaining bytes are saved in this array until logged next time.
  int16_t user_traffic_resource_type_bytes_
      [AppTabState::MAX][content::ResourceType::RESOURCE_TYPE_LAST_TYPE];
};

}  // namespace data_use_measurement

#endif  // COMPONENTS_DATA_USE_MEASUREMENT_CONTENT_CONTENT_URL_REQUEST_CLASSIFIER_H_
