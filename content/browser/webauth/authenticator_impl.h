// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_IMPL_H_
#define CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_IMPL_H_

#include <vector>

#include "base/cancelable_callback.h"
#include "content/common/content_export.h"
#include "device/u2f/u2f_request.h"
#include "device/u2f/u2f_return_code.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "third_party/WebKit/public/platform/modules/webauth/authenticator.mojom.h"
#include "url/origin.h"

namespace content {

class RenderFrameHost;

// Implementation of the public Authenticator interface.
class CONTENT_EXPORT AuthenticatorImpl : public webauth::mojom::Authenticator {
 public:
  static void Create(RenderFrameHost* render_frame_host,
                     webauth::mojom::AuthenticatorRequest request);
  ~AuthenticatorImpl() override;

  void set_connection_error_handler(const base::Closure& error_handler) {
    connection_error_handler_ = error_handler;
  }

 private:
  explicit AuthenticatorImpl(RenderFrameHost* render_frame_host);

  // mojom:Authenticator
  void MakeCredential(webauth::mojom::MakeCredentialOptionsPtr options,
                      MakeCredentialCallback callback) override;

  // Callback to handle the async response from a U2fDevice.
  void OnRegister(MakeCredentialCallback callback,
                  const std::string& client_data_json,
                  device::U2fReturnCode status_code,
                  const std::vector<uint8_t>& data);

  bool HasValidAlgorithm(
      const std::vector<webauth::mojom::PublicKeyCredentialParametersPtr>&
          parameters);

  void OnTimeout(MakeCredentialCallback callback);

  // As a result of a browser-side error or renderer-initiated mojo channel
  // closure (e.g. there was an error on the renderer side, or payment was
  // successful), this method is called. It is responsible for cleaning up.
  void OnConnectionTerminated();

  std::unique_ptr<device::U2fRequest> u2fRequest_;
  base::Closure connection_error_handler_;
  base::CancelableClosure timeout_callback_;
  url::Origin caller_origin_;
  base::WeakPtrFactory<AuthenticatorImpl> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(AuthenticatorImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_IMPL_H_
