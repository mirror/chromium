// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/protocol/http_ice_config_request.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/values.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "remoting/protocol/ice_config.h"

namespace remoting {
namespace protocol {

HttpIceConfigRequest::HttpIceConfigRequest(
    UrlRequestFactory* url_request_factory,
    const std::string& url,
    OAuthTokenGetter* oauth_token_getter)
    : url_(url), weak_factory_(this) {
}

HttpIceConfigRequest::~HttpIceConfigRequest() = default;

void HttpIceConfigRequest::Send(const OnIceConfigCallback& callback) {
  DCHECK(on_ice_config_callback_.is_null());
  DCHECK(!callback.is_null());

  on_ice_config_callback_ = callback;

  if (oauth_token_getter_) {
    oauth_token_getter_->CallWithToken(base::Bind(
        &HttpIceConfigRequest::OnOAuthToken, weak_factory_.GetWeakPtr()));
  } else {
    SendRequest();
  }
}

void HttpIceConfigRequest::OnOAuthToken(OAuthTokenGetter::Status status,
                                        const std::string& user_email,
                                        const std::string& access_token) {
  if (status != OAuthTokenGetter::SUCCESS) {
    LOG(ERROR) << "Failed to get OAuth token for IceConfig request.";
    base::ResetAndReturn(&on_ice_config_callback_).Run(IceConfig());
    return;
  }

  url_request_->AddHeader("Authorization:Bearer " + access_token);
  SendRequest();
}

void HttpIceConfigRequest::SendRequest() {
  url_request_->Start(
      base::Bind(&HttpIceConfigRequest::OnResponse, base::Unretained(this)));
}

void HttpIceConfigRequest::OnResponse(const UrlRequest::Result& result) {
  DCHECK(!on_ice_config_callback_.is_null());

  if (!result.success) {
    LOG(ERROR) << "Failed to fetch " << url_;
    base::ResetAndReturn(&on_ice_config_callback_).Run(IceConfig());
    return;
  }

  if (result.status != 200) {
    LOG(ERROR) << "Received status code " << result.status << " from " << url_
               << ": " << result.response_body;
    base::ResetAndReturn(&on_ice_config_callback_).Run(IceConfig());
    return;
  }

  IceConfig ice_config = IceConfig::Parse(result.response_body);
  if (ice_config.is_null()) {
    LOG(ERROR) << "Received invalid response from " << url_ << ": "
               << result.response_body;
  }

  base::ResetAndReturn(&on_ice_config_callback_).Run(ice_config);
}

}  // namespace protocol
}  // namespace remoting
