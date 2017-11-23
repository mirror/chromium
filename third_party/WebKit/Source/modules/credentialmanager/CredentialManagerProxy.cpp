// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/credentialmanager/CredentialManagerProxy.h"

#include "core/dom/Document.h"
#include "core/dom/ExecutionContext.h"
#include "core/frame/LocalFrame.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace blink {

CredentialManagerProxy::CredentialManagerProxy(Document* document) {
  LocalFrame* frame = document->GetFrame();
  SECURITY_CHECK(frame->IsMainFrame());
  frame->GetInterfaceProvider().GetInterface(&credential_manager_);
  frame->GetInterfaceProvider().GetInterface(
      mojo::MakeRequest(&authenticator_));
}

CredentialManagerProxy::~CredentialManagerProxy() {}

// static
CredentialManagerProxy* CredentialManagerProxy::From(Document* document) {
  if (!document->IsInMainFrame())
    return nullptr;
  if (!Supplement<Document>::From(document, SupplementName())) {
    ProvideTo(*document, CredentialManagerProxy::SupplementName(),
              new CredentialManagerProxy(document));
  }
  return static_cast<CredentialManagerProxy*>(
      Supplement<Document>::From(document, SupplementName()));
}

// static
CredentialManagerProxy* CredentialManagerProxy::From(
    ExecutionContext* execution_context) {
  if (!execution_context->IsDocument())
    return nullptr;
  return From(ToDocument(execution_context));
}

// static
const char* CredentialManagerProxy::SupplementName() {
  return "CredentialManagerProxy";
}

}  // namespace blink
