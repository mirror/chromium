// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/network_service_client.h"

#include "content/browser/ssl/ssl_client_auth_handler.h"
#include "content/browser/ssl/ssl_error_handler.h"
#include "content/browser/ssl/ssl_manager.h"
#include "content/browser/ssl_private_key_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/resource_request_info.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "net/ssl/client_cert_store.h"

namespace content {
namespace {

class SSLErrorDelegate : public SSLErrorHandler::Delegate {
 public:
  explicit SSLErrorDelegate(
      mojom::NetworkServiceClient::OnSSLCertificateErrorCallback response)
      : response_(std::move(response)), weak_factory_(this) {}
  ~SSLErrorDelegate() override {}
  void CancelSSLRequest(int error, const net::SSLInfo* ssl_info) override {
    std::move(response_).Run(error);
    delete this;
  }
  void ContinueSSLRequest() override {
    std::move(response_).Run(net::OK);
    delete this;
  }
  base::WeakPtr<SSLErrorDelegate> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

 private:
  mojom::NetworkServiceClient::OnSSLCertificateErrorCallback response_;
  base::WeakPtrFactory<SSLErrorDelegate> weak_factory_;
};

class SSLClientAuthDelegate : public SSLClientAuthHandler::Delegate {
 public:
  SSLClientAuthDelegate(
      mojom::NetworkServiceClient::OnCertificateRequestedCallback callback,
      ResourceRequestInfo::WebContentsGetter web_contents_getter,
      scoped_refptr<net::SSLCertRequestInfo> cert_info)
      : callback_(std::move(callback)), cert_info_(cert_info) {
    std::unique_ptr<net::ClientCertStore> client_cert_store;
    ssl_client_auth_handler_.reset(
        new SSLClientAuthHandler(std::move(client_cert_store),
                                 web_contents_getter, cert_info_.get(), this));
  }
  ~SSLClientAuthDelegate() override {}

  // SSLClientAuthHandler::Delegate:
  void ContinueWithCertificate(
      scoped_refptr<net::X509Certificate> cert,
      scoped_refptr<net::SSLPrivateKey> private_key) override {
    std::vector<uint16_t> algorithm_preferences =
        private_key->GetAlgorithmPreferences();
    mojom::SSLPrivateKeyPtr ssl_private_key;
    auto ssl_private_key_request = mojo::MakeRequest(&ssl_private_key);
    mojo::MakeStrongBinding(
        std::make_unique<SSLPrivateKeyImpl>(std::move(private_key)),
        std::move(ssl_private_key_request));
    std::move(callback_).Run(cert, algorithm_preferences,
                             std::move(ssl_private_key));
    ssl_client_auth_handler_->SelectCertificate();
    delete this;
  }

  // SSLClientAuthHandler::Delegate:
  void CancelCertificateSelection() override {
    std::move(callback_).Run(nullptr, std::vector<uint16_t>(), nullptr);
    delete this;
  }

 private:
  mojom::NetworkServiceClient::OnCertificateRequestedCallback callback_;
  scoped_refptr<net::SSLCertRequestInfo> cert_info_;
  std::unique_ptr<SSLClientAuthHandler> ssl_client_auth_handler_;
};

}  // namespace

NetworkServiceClient::NetworkServiceClient(
    mojom::NetworkServiceClientRequest network_service_client_request)
    : binding_(this, std::move(network_service_client_request)) {}

NetworkServiceClient::~NetworkServiceClient() = default;

void NetworkServiceClient::OnCertificateRequested(
    uint32_t process_id,
    uint32_t routing_id,
    const scoped_refptr<net::SSLCertRequestInfo>& cert_info,
    mojom::NetworkServiceClient::OnCertificateRequestedCallback callback) {
  base::Callback<WebContents*(void)> web_contents_getter =
      process_id ? base::Bind(WebContentsImpl::FromRenderFrameHostID,
                              process_id, routing_id)
                 : base::Bind(WebContents::FromFrameTreeNodeId, routing_id);
  new SSLClientAuthDelegate(std::move(callback), web_contents_getter,
                            cert_info);  // deletes self
}

void NetworkServiceClient::OnSSLCertificateError(
    ResourceType resource_type,
    const GURL& url,
    uint32_t process_id,
    uint32_t routing_id,
    const net::SSLInfo& ssl_info,
    bool fatal,
    OnSSLCertificateErrorCallback response) {
  SSLErrorDelegate* delegate =
      new SSLErrorDelegate(std::move(response));  // deletes self
  base::Callback<WebContents*(void)> web_contents_getter =
      process_id ? base::Bind(WebContentsImpl::FromRenderFrameHostID,
                              process_id, routing_id)
                 : base::Bind(WebContents::FromFrameTreeNodeId, routing_id);
  SSLManager::OnSSLCertificateError(delegate->GetWeakPtr(), resource_type, url,
                                    web_contents_getter, ssl_info, fatal);
}

}  // namespace content
