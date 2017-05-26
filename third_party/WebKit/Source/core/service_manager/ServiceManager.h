// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ServiceManager_h
#define ServiceManager_h

#include "core/mojo/MojoHandle.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class ServiceManager final : public GarbageCollected<ServiceManager>,
                             public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static void bindInterface(const String& service_name,
                            const String& interface_name,
                            MojoHandle* request_handle);

  DEFINE_INLINE_TRACE() {}
};

}  // namespace blink

#endif  // ServiceManager_h
