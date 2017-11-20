// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_IMPL_H_
#define CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_IMPL_H_

#include <vector>

#include "base/cancelable_callback.h"
#include "base/macros.h"
#include "content/common/content_export.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "third_party/WebKit/public/platform/modules/webauth/authenticator.mojom.h"
#include "url/origin.h"

namespace content {

class RenderFrameHost;

// Implementation of the public Authenticator interface.
class CONTENT_EXPORT AuthenticatorImpl : public webauth::mojom::Authenticator {
 public:
  explicit AuthenticatorImpl(RenderFrameHost* render_frame_host);
  ~AuthenticatorImpl() override;

  // Creates a binding between this object and |request|. Note that a
  // AuthenticatorImpl instance can be bound to multiple requests (as happens in
  // the case of simultaneous starting and finishing operations).
  void Bind(webauth::mojom::AuthenticatorRequest request);

 private:
  // mojom:Authenticator
  void MakeCredential(webauth::mojom::MakeCredentialOptionsPtr options,
                      MakeCredentialCallback callback) override;

  bool HasValidAlgorithm(
      const std::vector<webauth::mojom::PublicKeyCredentialParametersPtr>&
          parameters);

  // RAII binding of |this| to Authenticator requests.
  // "Owns pipes to this Authenticator from |render_frame_host_|."
  mojo::BindingSet<webauth::mojom::Authenticator> bindings_;
  base::CancelableClosure timeout_callback_;
  RenderFrameHost* render_frame_host_;

  DISALLOW_COPY_AND_ASSIGN(AuthenticatorImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_IMPL_H_
