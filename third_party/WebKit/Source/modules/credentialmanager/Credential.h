// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef Credential_h
#define Credential_h

#include "modules/ModulesExport.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/weborigin/KURL.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class ExceptionState;

class MODULES_EXPORT Credential : public GarbageCollectedFinalized<Credential>,
                                  public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  virtual ~Credential();

  virtual bool IsPassword() { return false; }
  virtual bool IsFederated() { return false; }

  // Credential.idl
  const String& id() const { return id_; }
  const String& type() const { return type_; }

  DEFINE_INLINE_VIRTUAL_TRACE() {}

 protected:
  Credential(const String& id);

  void SetType(const String& type) { type_ = type; }

  // Parses a string as a KURL. Throws an exception via |exceptionState| if an
  // invalid URL is produced.
  static KURL ParseStringAsURL(const String&, ExceptionState&);

  String id_;
  String type_;
};

}  // namespace blink

#endif  // Credential_h
