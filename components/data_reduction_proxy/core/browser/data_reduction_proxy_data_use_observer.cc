// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_data_use_observer.h"

#include "base/memory/ptr_util.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_config.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_configurator.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_io_data.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_metrics.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_util.h"
#include "components/data_reduction_proxy/core/common/lofi_decider.h"
#include "components/data_use_measurement/core/data_use.h"
#include "components/data_use_measurement/core/data_use_ascriber.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request.h"
#include "url/gurl.h"

namespace data_reduction_proxy {
namespace {

class MainFrameDataUseUntilCommitUserData
    : public base::SupportsUserData::Data {
 public:
  // Key used to store data usage in userdata until the page URL is available.
  static const void* const kUserDataKey;

  MainFrameDataUseUntilCommitUserData(
      int64_t network_bytes,
      int64_t original_bytes,
      DataReductionProxyRequestType request_type,
      const std::string& mime_type)
      : network_bytes_(network_bytes),
        original_bytes_(original_bytes),
        request_type_(request_type),
        mime_type_(mime_type) {}

  int64_t network_bytes() const { return network_bytes_; }
  int64_t original_bytes() const { return original_bytes_; }
  DataReductionProxyRequestType request_type() const { return request_type_; }
  const std::string& mime_type() const { return mime_type_; }

 private:
  int64_t network_bytes_;
  int64_t original_bytes_;
  DataReductionProxyRequestType request_type_;
  std::string mime_type_;
};

// Hostname used for the other bucket which consists of chrome-services traffic.
// This should be in sync with the same in DataReductionSiteBreakdownView.java
const char kOtherHostName[] = "Other";

// static
const void* const MainFrameDataUseUntilCommitUserData::kUserDataKey =
    &MainFrameDataUseUntilCommitUserData::kUserDataKey;

}  // namespace

DataReductionProxyDataUseObserver::DataReductionProxyDataUseObserver(
    DataReductionProxyIOData* data_reduction_proxy_io_data,
    data_use_measurement::DataUseAscriber* data_use_ascriber)
    : data_reduction_proxy_io_data_(data_reduction_proxy_io_data),
      data_use_ascriber_(data_use_ascriber) {
  DCHECK(data_reduction_proxy_io_data_);
  data_use_ascriber_->AddObserver(this);
}

DataReductionProxyDataUseObserver::~DataReductionProxyDataUseObserver() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  data_use_ascriber_->RemoveObserver(this);
}

void DataReductionProxyDataUseObserver::OnPageLoadCommit(
    data_use_measurement::DataUse* data_use) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(data_use->url().is_valid());
  MainFrameDataUseUntilCommitUserData* data =
      reinterpret_cast<MainFrameDataUseUntilCommitUserData*>(
          data_use->GetUserData(
              MainFrameDataUseUntilCommitUserData::kUserDataKey));
  if (data) {
    // Record the data use bytes saved in user data to database.
    data_reduction_proxy_io_data_->UpdateDataUse(
        data->network_bytes(), data->original_bytes(),
        data_use->url().HostNoBrackets(),
        data_reduction_proxy_io_data_->IsEnabled(), data->request_type(),
        data->mime_type());
    data_use->RemoveUserData(MainFrameDataUseUntilCommitUserData::kUserDataKey);
  }
}

void DataReductionProxyDataUseObserver::OnPageResourceLoad(
    const net::URLRequest& request,
    data_use_measurement::DataUse* data_use) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (!request.url().SchemeIs(url::kHttpsScheme) &&
      !request.url().SchemeIs(url::kHttpScheme)) {
    return;
  }

  if (request.GetTotalReceivedBytes() <= 0)
    return;

  int64_t network_bytes = request.GetTotalReceivedBytes();
  DataReductionProxyRequestType request_type = GetDataReductionProxyRequestType(
      request, data_reduction_proxy_io_data_->configurator()->GetProxyConfig(),
      *data_reduction_proxy_io_data_->config());

  // Estimate how many bytes would have been used if the DataReductionProxy was
  // not used, and record the data usage.
  int64_t original_bytes = util::EstimateOriginalReceivedBytes(
      request, request_type == VIA_DATA_REDUCTION_PROXY,
      data_reduction_proxy_io_data_->lofi_decider());

  std::string mime_type;
  if (request.response_headers())
    request.response_headers()->GetMimeType(&mime_type);

  if (data_use->traffic_type() ==
          data_use_measurement::DataUse::TrafficType::USER_TRAFFIC &&
      !data_use->url().is_valid()) {
    // URL will be empty until pageload navigation commits. Save the data use of
    // the mainframe until then. No sub-resource requests can be finished until
    // commit.
    DCHECK(!data_use->GetUserData(
        MainFrameDataUseUntilCommitUserData::kUserDataKey));
    data_use->SetUserData(
        MainFrameDataUseUntilCommitUserData::kUserDataKey,
        base::MakeUnique<MainFrameDataUseUntilCommitUserData>(
            network_bytes, original_bytes, request_type, mime_type));
  } else {
    data_reduction_proxy_io_data_->UpdateDataUse(
        network_bytes, original_bytes,
        data_use->traffic_type() ==
                data_use_measurement::DataUse::TrafficType::USER_TRAFFIC
            ? data_use->url().HostNoBrackets()
            : kOtherHostName,
        data_reduction_proxy_io_data_->IsEnabled(), request_type, mime_type);
  }
}

void DataReductionProxyDataUseObserver::OnPageDidFinishLoad(
    data_use_measurement::DataUse* data_use) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  // TODO(dougarnett): 781885 - estimate data saving for NoScript.
}

}  // namespace data_reduction_proxy
