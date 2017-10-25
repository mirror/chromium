// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_IMPL_H_
#define CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_IMPL_H_

#include <memory>
#include <vector>

#include "base/cancelable_callback.h"
#include "base/macros.h"
#include "content/browser/webauth/register_response_data.h"
#include "content/common/content_export.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "third_party/WebKit/public/platform/modules/webauth/authenticator.mojom.h"
#include "url/origin.h"

namespace device {
class U2fRequest;
enum class U2fReturnCode : uint8_t;
}  // namespace device

namespace content {

class RenderFrameHost;

// Implementation of the public Authenticator interface.
class CONTENT_EXPORT AuthenticatorImpl : public webauth::mojom::Authenticator {
 public:
  static void Create(RenderFrameHost* render_frame_host,
                     webauth::mojom::AuthenticatorRequest request);
  ~AuthenticatorImpl() override;

 private:
  explicit AuthenticatorImpl(RenderFrameHost* render_frame_host);

  // mojom:Authenticator
  void MakeCredential(webauth::mojom::MakeCredentialOptionsPtr options,
                      MakeCredentialCallback callback) override;

  // Callback to handle the async response from a U2fDevice.
  void OnDeviceResponse(MakeCredentialCallback callback,
                        std::unique_ptr<CollectedClientData> client_data,
                        device::U2fReturnCode status_code,
                        const std::vector<uint8_t>& data);

  webauth::mojom::PublicKeyCredentialInfoPtr CreatePublicKeyCredentialInfo(
      std::unique_ptr<RegisterResponseData> data);

  bool HasValidAlgorithm(
      const std::vector<webauth::mojom::PublicKeyCredentialParametersPtr>&
          parameters);

  void OnTimeout(
      base::OnceCallback<void(webauth::mojom::AuthenticatorStatus,
                              webauth::mojom::PublicKeyCredentialInfoPtr)>
          callback);

  // As a result of a browser-side error or renderer-initiated mojo channel
  // closure (e.g. there was an error on the renderer side, or the request was
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
