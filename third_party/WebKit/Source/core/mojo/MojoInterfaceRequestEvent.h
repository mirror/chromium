// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MojoInterfaceRequestEvent_h
#define MojoInterfaceRequestEvent_h

#include "core/events/Event.h"
#include "core/mojo/MojoInterfaceRequestEventInit.h"

namespace blink {

class MojoHandle;

class MojoInterfaceRequestEvent final : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  ~MojoInterfaceRequestEvent() override;

  static MojoInterfaceRequestEvent* Create(MojoHandle* handle) {
    return new MojoInterfaceRequestEvent(handle);
  }

  static MojoInterfaceRequestEvent* Create(
      const AtomicString& type,
      const MojoInterfaceRequestEventInit& initializer) {
    return new MojoInterfaceRequestEvent(type, initializer);
  }

  MojoHandle* handle() const { return handle_; }

  const AtomicString& InterfaceName() const override {
    return EventNames::MojoInterfaceRequestEvent;
  }

  DECLARE_VIRTUAL_TRACE();

 private:
  explicit MojoInterfaceRequestEvent(MojoHandle*);
  MojoInterfaceRequestEvent(const AtomicString& type,
                            const MojoInterfaceRequestEventInit&);

  Member<MojoHandle> handle_;
};

}  // namespace blink

#endif  // MojoInterfaceRequestEvent_h
