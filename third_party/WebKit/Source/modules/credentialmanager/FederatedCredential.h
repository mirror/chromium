// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FederatedCredential_h
#define FederatedCredential_h

#include "bindings/core/v8/serialization/SerializedScriptValue.h"
#include "modules/ModulesExport.h"
#include "modules/credentialmanager/Credential.h"
#include "platform/heap/Handle.h"
#include "platform/weborigin/KURL.h"
#include "platform/weborigin/SecurityOrigin.h"
#include "platform/wtf/RefPtr.h"
#include "platform/wtf/text/WTFString.h"

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
  const String provider() const { return provider_->ToString(); }
  const String& name() const { return name_; }
  const KURL& iconURL() const { return icon_url_; }

  // TODO(mkwst): This is a stub, as we don't yet have any support on the
  // Chromium-side.
  const String& protocol() const { return g_empty_string; }

  DECLARE_VIRTUAL_TRACE();

 private:
  FederatedCredential(const String& id,
                      RefPtr<SecurityOrigin> provider,
                      const String& name,
                      const KURL& icon);
  String name_;
  KURL icon_url_;
  RefPtr<SecurityOrigin> provider_;
};

}  // namespace blink

#endif  // FederatedCredential_h
