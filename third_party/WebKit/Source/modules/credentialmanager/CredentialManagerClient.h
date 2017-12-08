// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CredentialManagerClient_h
#define CredentialManagerClient_h

#include <memory>
#include "core/page/Page.h"
#include "modules/ModulesExport.h"
#include "modules/credentialmanager/MakePublicKeyCredentialOptions.h"
#include "modules/credentialmanager/WebAuthenticationClient.h"
#include "platform/Supplementable.h"
#include "public/platform/WebCredentialMediationRequirement.h"
#include "public/platform/modules/credentialmanager/credential_manager.mojom-blink.h"

namespace blink {

class ExecutionContext;
class Page;
class WebCredential;
class WebURL;

// CredentialManagerClient lives as a supplement to Page, and wraps the Mojo
// calls.
class MODULES_EXPORT CredentialManagerClient
    : public GarbageCollectedFinalized<CredentialManagerClient>,
      public Supplement<Page> {
  USING_GARBAGE_COLLECTED_MIXIN(CredentialManagerClient);

 public:
  typedef WebCallbacks<std::unique_ptr<WebCredential>,
                       WebCredentialManagerError>
      RequestCallbacks;
  typedef WebCallbacks<void, WebCredentialManagerError> NotificationCallbacks;

  explicit CredentialManagerClient(Page&);
  virtual ~CredentialManagerClient();
  virtual void Trace(blink::Visitor*);

  static const char* SupplementName();
  static CredentialManagerClient* From(Page*);
  static CredentialManagerClient* From(ExecutionContext*);

  // Ownership of the callback is transferred to the callee for each of
  // the following methods.
  virtual void DispatchStore(const WebCredential&, NotificationCallbacks*);
  virtual void DispatchPreventSilentAccess(NotificationCallbacks*);
  virtual void DispatchGet(WebCredentialMediationRequirement,
                           bool include_passwords,
                           const WTF::Vector<KURL>& federations,
                           RequestCallbacks*);
  virtual void DispatchMakeCredential(
      LocalFrame&,
      const MakePublicKeyCredentialOptions&,
      std::unique_ptr<WebAuthenticationClient::PublicKeyCallbacks>);

 private:
  void ConnectToMojoCMIfNeeded();
  void OnMojoConnectionError();

  // TODO(crbug.com/740081): Merge authentication_client_ into this class.
  Member<WebAuthenticationClient> authentication_client_;

  // Pointer to the browser implementation of the credential manager.
  password_manager::mojom::blink::CredentialManagerAssociatedPtr
      mojo_cm_service_;
};

MODULES_EXPORT void ProvideCredentialManagerClientTo(Page&,
                                                     CredentialManagerClient*);

}  // namespace blink

#endif  // CredentialManagerClient_h
