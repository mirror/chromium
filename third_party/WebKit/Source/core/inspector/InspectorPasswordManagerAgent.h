// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef InspectorPasswordManagerAgent_h
#define InspectorPasswordManagerAgent_h

#include "core/CoreExport.h"
#include "core/inspector/InspectorBaseAgent.h"
#include "core/inspector/protocol/PasswordManager.h"

namespace blink {

class CORE_EXPORT InspectorPasswordManagerAgent
    : public InspectorBaseAgent<protocol::PasswordManager::Metainfo> {
  // WTF_MAKE_NONCOPYABLE(InspectorPasswordManagerAgent);

 public:
  InspectorPasswordManagerAgent();
  // DECLARE_VIRTUAL_TRACE();

  void TrivialPasswordManagerEventFired(LocalFrame*);

  protocol::Response enable() override;

 private:
};

}  // namespace blink

#endif  // !defined(InspectorPasswordManagerAgent_h)
