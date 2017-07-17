// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FederatedCredential_h
#define FederatedCredential_h

#include "bindings/core/v8/serialization/SerializedScriptValue.h"
#include "modules/ModulesExport.h"
#include "modules/credentialmanager/Credential.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/weborigin/KURL.h"
#include "platform/weborigin/SecurityOrigin.h"

namespace blink {

class FederatedCredentialInit;

class MODULES_EXPORT FederatedCredential final : public Credential {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static FederatedCredential* Create(const String& id,
                                     RefPtr<SecurityOrigin> provider,
                                     const String& name,
                                     const KURL& icon);
  static FederatedCredential* Create(const FederatedCredentialInit&,
                                     ExceptionState&);

  // FederatedCredential.idl
  const String provider() const;
  const String& name() const;
  const KURL& iconURL() const;

  // TODO(mkwst): This is a stub, as we don't yet have any support on the
  // Chromium-side.
  const String& protocol() const { return g_empty_string; }

 private:
  FederatedCredential(const String& id,
                      RefPtr<SecurityOrigin> provider,
                      const String& name,
                      const KURL& icon);
};

}  // namespace blink

#endif  // FederatedCredential_h
