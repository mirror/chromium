// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CredentialManagerProxy_h
#define CredentialManagerProxy_h

#include "modules/ModulesExport.h"
#include "modules/credentialmanager/WebAuthenticationClient.h"
#include "platform/Supplementable.h"
#include "platform/heap/Handle.h"
#include "public/platform/modules/credentialmanager/credential_manager.mojom-blink.h"

namespace blink {

class ExecutionContext;
class Document;

// Owns the client end of the interface connection to a mojo::CredentialManager
// implementation that services requests in the security context of the document
// supplemented by this CredentialManagerProxy instance.
//
// This facilitates routing API calls to be serviced in the correct security
// context, even if the `window.navigator.credentials` instance from one
// browsing context was passed to another; in which case the Credential Manager
// API call must be serviced in the browsing context responsible for actually
// calling the API method, as opposed to the browsing context whose global
// object owns the CredentialsContainer instance on which the method was called.
class MODULES_EXPORT CredentialManagerProxy
    : public GarbageCollectedFinalized<CredentialManagerProxy>,
      public Supplement<Document> {
  USING_GARBAGE_COLLECTED_MIXIN(CredentialManagerProxy);

 public:
  explicit CredentialManagerProxy(Document*);
  virtual ~CredentialManagerProxy();
  virtual void Trace(blink::Visitor*);

  ::password_manager::mojom::blink::CredentialManager* Service();

  static CredentialManagerProxy* From(ExecutionContext*);
  static CredentialManagerProxy* From(Document*);

 private:
  static const char* SupplementName();

  ::password_manager::mojom::blink::CredentialManagerPtr mojo_cm_service_;

  // TODO(crbug.com/740081): Merge authentication_client_ into this class.
  Member<WebAuthenticationClient> authentication_client_;
};

}  // namespace blink

#endif  // CredentialManagerProxy_h
