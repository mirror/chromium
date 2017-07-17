// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PasswordCredential_h
#define PasswordCredential_h

#include "bindings/modules/v8/FormDataOrURLSearchParams.h"
#include "modules/ModulesExport.h"
#include "modules/credentialmanager/Credential.h"
#include "platform/heap/Handle.h"
#include "platform/network/EncodedFormData.h"
#include "platform/weborigin/KURL.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class HTMLFormElement;
class PasswordCredentialData;

using CredentialPostBodyType = FormDataOrURLSearchParams;

class MODULES_EXPORT PasswordCredential final : public Credential {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static PasswordCredential* Create(const String& id,
                                    const String& password,
                                    const String& name,
                                    const KURL& icon);
  static PasswordCredential* Create(const PasswordCredentialData&,
                                    ExceptionState&);
  static PasswordCredential* Create(HTMLFormElement*, ExceptionState&);

  bool IsPassword() override { return true; }

  // PasswordCredential.idl
  void setIdName(const String& name) { id_name_ = name; }
  const String& idName() const { return id_name_; }

  void setPasswordName(const String& name) { password_name_ = name; }
  const String& passwordName() const { return password_name_; }

  void setAdditionalData(const CredentialPostBodyType& data) {
    additional_data_ = data;
  }
  void additionalData(CredentialPostBodyType& out) const {
    out = additional_data_;
  }

  const String& password() const { return password_; }
  const String& name() const { return name_; }
  const KURL& iconURL() const { return icon_url_; }

  // Internal methods
  PassRefPtr<EncodedFormData> EncodeFormData(String& content_type) const;
  DECLARE_VIRTUAL_TRACE();

 private:
  PasswordCredential(const String& id,
                     const String& password,
                     const String& name,
                     const KURL& icon);

  String name_;
  KURL icon_url_;
  String password_;
  String id_name_;
  String password_name_;
  CredentialPostBodyType additional_data_;
};

}  // namespace blink

#endif  // PasswordCredential_h
