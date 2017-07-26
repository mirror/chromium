// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebAuthenticationClient_h
#define WebAuthenticationClient_h

#include "platform/heap/Handle.h"
#include "public/platform/WebCallbacks.h"
#include "public/platform/WebCredentialManagerError.h"
#include "public/platform/modules/webauth/authenticator.mojom-blink.h"

namespace blink {

class LocalFrame;
class MakeCredentialOptions;
class PublicKeyCredentialRequestOptions;

class WebAuthenticationClient final
    : public GarbageCollectedFinalized<WebAuthenticationClient> {
 public:
  // Used by calls to authenticator.mojom.
  typedef WebCallbacks<webauth::mojom::blink::PublicKeyCredentialInfoPtr,
                       WebCredentialManagerError>
      PublicKeyCreateCallbacks;

  explicit WebAuthenticationClient(LocalFrame&);

  virtual ~WebAuthenticationClient();

  // WebAuthentication.idl
  void DispatchMakeCredential(const MakeCredentialOptions&,
                              PublicKeyCreateCallbacks*);

  void getAssertion(const PublicKeyCredentialRequestOptions&,
                    PublicKeyCreateCallbacks*);

  DECLARE_VIRTUAL_TRACE();

 private:
  void OnMakeCredential(PublicKeyCreateCallbacks*,
                        webauth::mojom::blink::AuthenticatorStatus,
                        webauth::mojom::blink::PublicKeyCredentialInfoPtr);
  void OnAuthenticatorConnectionError();
  void Cleanup();
  webauth::mojom::blink::AuthenticatorPtr authenticator_;
};
}  // namespace blink

#endif  // WebAuthenticationClient_h
