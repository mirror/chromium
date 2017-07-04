// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SafeHTML_h
#define SafeHTML_h

#include "core/CoreExport.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class ExceptionState;
class ScriptPromise;
class ScriptState;

class CORE_EXPORT SafeHTML final : public GarbageCollectedFinalized<SafeHTML>,
                                   public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static SafeHTML* Create(const String& html) { return new SafeHTML(html); }

  // CredentialsContainer.idl
  String toString() const;
  static ScriptPromise escape(ScriptState*, const String& html);
  static ScriptPromise unsafelyCreate(ScriptState*, const String& html);

  DEFINE_INLINE_VIRTUAL_TRACE() {}

 private:
  SafeHTML(const String& html);

  const String html_;
};

}  // namespace blink

#endif  // SafeHTML_h
