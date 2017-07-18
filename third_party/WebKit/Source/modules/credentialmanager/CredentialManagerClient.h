// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CredentialManagerClient_h
#define CredentialManagerClient_h

#include "modules/ModulesExport.h"
#include "platform/Supplementable.h"
#include "platform/weborigin/KURL.h"
#include "public/platform/WebCallbacks.h"
#include "public/platform/WebCredentialManagerError.h"
#include "public/platform/WebCredentialMediationRequirement.h"
#include "public/platform/WebVector.h"
#include "third_party/WebKit/public/platform/modules/credentialmanager/credential_manager.mojom-blink.h"

namespace blink {

class Credential;
class ExecutionContext;

// CredentialManagerClient lives as a supplement to Document, and wraps the
// embedder-provided WebCredentialManagerClient's methods to make them visible
// to the bindings code.
class MODULES_EXPORT CredentialManagerClient
    : public GarbageCollectedFinalized<CredentialManagerClient>,
      public Supplement<ExecutionContext> {
  USING_GARBAGE_COLLECTED_MIXIN(CredentialManagerClient);

 public:
  using RequestCallbacks = WebCallbacks<Credential*, WebCredentialManagerError>;
  using NotificationCallbacks = WebCallbacks<void, WebCredentialManagerError>;

  explicit CredentialManagerClient(ExecutionContext*);
  virtual ~CredentialManagerClient();
  DECLARE_VIRTUAL_TRACE();

  static const char* SupplementName();
  static CredentialManagerClient* From(ExecutionContext*);
  static void ProvideToExecutionContext(ExecutionContext*,
                                        CredentialManagerClient*);

  // Ownership of the callback is transferred to the callee for each of
  // the following methods.
  virtual void DispatchFailedSignIn(Credential*, NotificationCallbacks*);
  virtual void DispatchStore(Credential*, NotificationCallbacks*);
  virtual void DispatchPreventSilentAccess(NotificationCallbacks*);
  virtual void DispatchGet(WebCredentialMediationRequirement,
                           bool include_passwords,
                           const ::WTF::Vector<KURL>& federations,
                           RequestCallbacks*);

 private:
  password_manager::mojom::blink::CredentialManagerPtr mojo_cm_service_;
};

}  // namespace blink

#endif  // CredentialManagerClient_h
