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
#include "net/ssl/client_cert_store.h"

#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {
namespace {

class SSLDelegate : public SSLErrorHandler::Delegate {
 public:
  explicit SSLDelegate(
      mojom::NetworkServiceClient::OnSSLCertificateErrorCallback response)
      : response_(std::move(response)), weak_factory_(this) {}
  ~SSLDelegate() override {}
  void CancelSSLRequest(int error, const net::SSLInfo* ssl_info) override {
    std::move(response_).Run(error);
    delete this;
  }
  void ContinueSSLRequest() override {
    std::move(response_).Run(net::OK);
    delete this;
  }
  base::WeakPtr<SSLDelegate> GetWeakPtr() { return weak_factory_.GetWeakPtr(); }

 private:
  mojom::NetworkServiceClient::OnSSLCertificateErrorCallback response_;
  base::WeakPtrFactory<SSLDelegate> weak_factory_;
};

class SSLClientDelegate : public SSLClientAuthHandler::Delegate {
 public:
  SSLClientDelegate(
      mojom::NetworkServiceClient::OnCertificateRequestedCallback callback,
      ResourceRequestInfo::WebContentsGetter web_contents_getter,
      scoped_refptr<net::SSLCertRequestInfo> cert_info,
      mojom::SSLPrivateKeyRequest ssl_private_key_request)
      : callback_(std::move(callback)),
        cert_info_(cert_info),
        ssl_private_key_request_(std::move(ssl_private_key_request)),
        weak_factory_(this) {
    std::unique_ptr<net::ClientCertStore> client_cert_store;
    ssl_client_auth_handler_.reset(
        new SSLClientAuthHandler(std::move(client_cert_store),
                                 web_contents_getter, cert_info_.get(), this));
  }
  ~SSLClientDelegate() override {}

  // SSLClientAuthHandler::Delegate:
  void ContinueWithCertificate(
      scoped_refptr<net::X509Certificate> cert,
      scoped_refptr<net::SSLPrivateKey> private_key) override {
    std::vector<uint16_t> algorithm_preferences =
        private_key->GetAlgorithmPreferences();
    mojo::MakeStrongBinding(
        std::make_unique<SSLPrivateKeyImpl>(std::move(private_key)),
        std::move(ssl_private_key_request_));
    std::move(callback_).Run(cert, algorithm_preferences);
    ssl_client_auth_handler_->SelectCertificate();
    delete this;
  }

  // SSLClientAuthHandler::Delegate:
  void CancelCertificateSelection() override {
    std::move(callback_).Run(nullptr, std::vector<uint16_t>());
    delete this;
  }

  base::WeakPtr<SSLClientDelegate> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

 private:
  mojom::NetworkServiceClient::OnCertificateRequestedCallback callback_;
  scoped_refptr<net::SSLCertRequestInfo> cert_info_;
  std::unique_ptr<SSLClientAuthHandler> ssl_client_auth_handler_;
  mojom::SSLPrivateKeyRequest ssl_private_key_request_;
  base::WeakPtrFactory<SSLClientDelegate> weak_factory_;
};

}  // namespace

NetworkServiceClient::NetworkServiceClient(
    mojom::NetworkServiceClientRequest network_service_client_request)
    : binding_(this, std::move(network_service_client_request)) {}

NetworkServiceClient::~NetworkServiceClient() = default;

void NetworkServiceClient::OnCertificateRequested(
    ResourceType resource_type,
    const GURL& url,
    uint32_t process_id,
    uint32_t routing_id,
    const scoped_refptr<net::SSLCertRequestInfo>& cert_info,
    mojom::SSLPrivateKeyRequest ssl_private_key_request,
    mojom::NetworkServiceClient::OnCertificateRequestedCallback callback) {
  base::Callback<WebContents*(void)> web_contents_getter =
      process_id ? base::Bind(WebContentsImpl::FromRenderFrameHostID,
                              process_id, routing_id)
                 : base::Bind(WebContents::FromFrameTreeNodeId, routing_id);
  new SSLClientDelegate(std::move(callback), web_contents_getter, cert_info,
                        std::move(ssl_private_key_request));  // deletes self
}

void NetworkServiceClient::OnSSLCertificateError(
    ResourceType resource_type,
    const GURL& url,
    uint32_t process_id,
    uint32_t routing_id,
    const net::SSLInfo& ssl_info,
    bool fatal,
    OnSSLCertificateErrorCallback response) {
  SSLDelegate* delegate = new SSLDelegate(std::move(response));  // deletes self
  base::Callback<WebContents*(void)> web_contents_getter =
      process_id ? base::Bind(WebContentsImpl::FromRenderFrameHostID,
                              process_id, routing_id)
                 : base::Bind(WebContents::FromFrameTreeNodeId, routing_id);
  SSLManager::OnSSLCertificateError(delegate->GetWeakPtr(), resource_type, url,
                                    web_contents_getter, ssl_info, fatal);
}

}  // namespace content
