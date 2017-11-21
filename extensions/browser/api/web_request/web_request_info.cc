// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/web_request/web_request_info.h"

#include <memory>
#include <string>

#include "base/values.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/browser/websocket_handshake_request_info.h"
#include "extensions/browser/api/web_request/upload_data_presenter.h"
#include "extensions/browser/api/web_request/web_request_api_constants.h"
#include "extensions/browser/extension_api_frame_id_map.h"
#include "extensions/browser/extension_navigation_ui_data.h"
#include "extensions/browser/extensions_browser_client.h"
#include "net/base/upload_data_stream.h"
#include "net/log/net_log_with_source.h"
#include "net/url_request/url_request.h"

namespace keys = extension_web_request_api_constants;

namespace extensions {

namespace {

std::unique_ptr<base::Value> NetLogExtensionIdCallback(
    const std::string& extension_id,
    net::NetLogCaptureMode capture_mode) {
  auto params = std::make_unique<base::DictionaryValue>();
  params->SetString("extension_id", extension_id);
  return params;
}

// Implements Logger using NetLog, mirroring the logging facilities used prior
// to the introduction of WebRequestInfo.
// TODO(crbug.com/721414): Transition away from using NetLog.
class NetLogLogger : public WebRequestInfo::Logger {
 public:
  explicit NetLogLogger(net::URLRequest* request) : request_(request) {}
  ~NetLogLogger() override = default;

  // WebRequestInfo::Logger:
  void LogEvent(net::NetLogEventType event_type,
                const std::string& extension_id) override {
    request_->net_log().AddEvent(
        event_type, base::Bind(&NetLogExtensionIdCallback, extension_id));
  }

  void LogBlockedBy(const std::string& blocker_info) override {
    request_->LogAndReportBlockedBy(blocker_info.c_str());
  }

  void LogUnblocked() override { request_->LogUnblocked(); }

 private:
  net::URLRequest* const request_;

  DISALLOW_COPY_AND_ASSIGN(NetLogLogger);
};

}  // namespace

WebRequestInfo::WebRequestInfo() = default;

WebRequestInfo::WebRequestInfo(net::URLRequest* url_request) {
  id = url_request->identifier();
  url = url_request->url();
  site_for_cookies = url_request->site_for_cookies();
  method = url_request->method();
  initiator = url_request->initiator();
  extra_request_headers = url_request->extra_request_headers();

  const content::ResourceRequestInfo* info =
      content::ResourceRequestInfo::ForRequest(url_request);
  if (url.SchemeIsWSOrWSS())
    web_request_type = WebRequestResourceType::WEB_SOCKET;

  if (info) {
    child_id = info->GetChildID();
    routing_id = info->GetRenderFrameID();
    type = info->GetResourceType();
    if (web_request_type == WebRequestResourceType::OTHER)
      web_request_type = ToWebRequestResourceType(type.value());
    is_async = info->IsAsync();
  } else if (web_request_type == WebRequestResourceType::WEB_SOCKET) {
    // TODO(pkalinnikov): Consider embedding WebSocketHandshakeRequestInfo into
    // UrlRequestUserData.
    const content::WebSocketHandshakeRequestInfo* ws_info =
        content::WebSocketHandshakeRequestInfo::ForRequest(url_request);
    if (ws_info) {
      child_id = ws_info->GetChildId();
      routing_id = ws_info->GetRenderFrameId();
    }
  } else {
    content::ResourceRequestInfo::GetRenderFrameForRequest(
        url_request, &child_id, &routing_id);
  }

  const net::UploadDataStream* upload_data = url_request->get_upload();
  if (upload_data && (method == "POST" || method == "PUT")) {
    request_body_data = std::make_unique<base::DictionaryValue>();

    // Get the data presenters, ordered by how specific they are.
    ParsedDataPresenter parsed_data_presenter(*url_request);
    RawDataPresenter raw_data_presenter;
    UploadDataPresenter* const presenters[] = {
        &parsed_data_presenter,  // 1: any parseable forms? (Specific to forms.)
        &raw_data_presenter      // 2: any data at all? (Non-specific.)
    };
    // Keys for the results of the corresponding presenters.
    static const char* const kKeys[] = {keys::kRequestBodyFormDataKey,
                                        keys::kRequestBodyRawKey};

    const std::vector<std::unique_ptr<net::UploadElementReader>>* readers =
        upload_data->GetElementReaders();
    bool some_succeeded = false;
    if (readers) {
      for (size_t i = 0; i < arraysize(presenters); ++i) {
        for (const auto& reader : *readers)
          presenters[i]->FeedNext(*reader);
        if (presenters[i]->Succeeded()) {
          request_body_data->Set(kKeys[i], presenters[i]->Result());
          some_succeeded = true;
          break;
        }
      }
    }
    if (!some_succeeded) {
      request_body_data->SetString(keys::kRequestBodyErrorKey,
                                   "Unknown error.");
    }
  }

  ExtensionsBrowserClient* browser_client = ExtensionsBrowserClient::Get();
  ExtensionNavigationUIData* navigation_ui_data = nullptr;
  if (browser_client) {
    navigation_ui_data =
        browser_client->GetExtensionNavigationUIData(url_request);
  }

  if (navigation_ui_data) {
    is_browser_side_navigation = true;
    is_web_view = navigation_ui_data->is_web_view();
    web_view_instance_id = navigation_ui_data->web_view_instance_id();
    web_view_rules_registry_id =
        navigation_ui_data->web_view_rules_registry_id();
    frame_data = navigation_ui_data->frame_data();
  } else if (routing_id >= 0) {
    ExtensionApiFrameIdMap::FrameData data;
    bool was_cached = ExtensionApiFrameIdMap::Get()->GetCachedFrameDataOnIO(
        child_id, routing_id, &data);
    if (was_cached)
      frame_data = data;
  }

  logger = std::make_unique<NetLogLogger>(url_request);
}

WebRequestInfo::~WebRequestInfo() = default;

void WebRequestInfo::AddResponseInfoFromURLRequest(
    net::URLRequest* url_request) {
  response_code = url_request->GetResponseCode();
  response_headers = url_request->response_headers();
  response_ip = url_request->GetSocketAddress().host();
  response_from_cache = url_request->was_cached();
}

}  // namespace extensions
