// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_REDUCTION_PROXY_CONTENT_BROWSER_CONTENT_LOFI_DECIDER_H_
#define COMPONENTS_DATA_REDUCTION_PROXY_CONTENT_BROWSER_CONTENT_LOFI_DECIDER_H_

#include <string>

#include "base/macros.h"
#include "base/supports_user_data.h"
#include "components/data_reduction_proxy/core/common/lofi_decider.h"
#include "url/gurl.h"

namespace net {
class HttpRequestHeaders;
class URLRequest;
}

namespace previews {
class PreviewsDecider;
}

namespace data_reduction_proxy {

// Holds information about the AMP redirection previews applied to a main frame
// URLRequest.
class AMPRedirectionPreviewsUserData : public base::SupportsUserData::Data {
 public:
  explicit AMPRedirectionPreviewsUserData(const GURL& url) : orig_url_(url) {}

  ~AMPRedirectionPreviewsUserData() override {}

  const GURL& orig_url() { return orig_url_; }

  static const void* const kAMPRedirectionPreviewsUserDataKey;

 private:
  // Original URL before previews transformation is applied.
  const GURL orig_url_;
};

// Class responsible for deciding whether a request should be requested with low
// fidelity (Lo-Fi) or not. Relies on the Lo-Fi mode state stored in the
// request's content::ResourceRequestInfo, which must be fetched using
// content::ResourceRequestInfo::ForRequest. Lo-Fi mode will not be enabled for
// requests that don't have a ResourceRequestInfo, such as background requests.
// Owned by DataReductionProxyIOData and should be called on the IO thread.
class ContentLoFiDecider : public LoFiDecider {
 public:
  ContentLoFiDecider();
  ~ContentLoFiDecider() override;

  // LoFiDecider implementation:
  bool IsUsingLoFi(const net::URLRequest& request) const override;
  void MaybeSetAcceptTransformHeader(
      const net::URLRequest& request,
      net::HttpRequestHeaders* headers) const override;
  bool IsSlowPagePreviewRequested(
      const net::HttpRequestHeaders& headers) const override;
  bool IsLitePagePreviewRequested(
      const net::HttpRequestHeaders& headers) const override;
  void RemoveAcceptTransformHeader(
      net::HttpRequestHeaders* headers) const override;
  bool ShouldRecordLoFiUMA(const net::URLRequest& request) const override;
  bool IsClientLoFiImageRequest(const net::URLRequest& request) const override;
  bool IsClientLoFiAutoReloadRequest(
      const net::URLRequest& request) const override;

  void MaybeApplyAMPPreview(
      net::URLRequest* request,
      GURL* new_url,
      previews::PreviewsDecider* previews_decider) const override;

  void MaybeContinueOrDisableAMPPreview(
      net::URLRequest* request,
      int response_code,
      LoFiUIService* lofi_ui_service) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ContentLoFiDecider);
};

}  // namespace data_reduction_proxy

#endif  // COMPONENTS_DATA_REDUCTION_PROXY_CONTENT_BROWSER_CONTENT_LOFI_DECIDER_H_
