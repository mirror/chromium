// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/testing/NullExecutionContext.h"

#include "core/dom/events/Event.h"
#include "core/frame/DOMTimer.h"
#include "core/frame/csp/ContentSecurityPolicy.h"
#include "public/platform/Platform.h"

namespace blink {

namespace {

class NullEventQueue final : public EventQueue {
 public:
  NullEventQueue() = default;
  ~NullEventQueue() override = default;
  bool EnqueueEvent(const base::Location&, Event*) override { return true; }
  bool CancelEvent(Event*) override { return true; }
  void Close() override {}
};

}  // namespace

NullExecutionContext::NullExecutionContext()
    : tasks_need_pause_(false),
      is_secure_context_(true),
      queue_(new NullEventQueue()) {}

void NullExecutionContext::SetIsSecureContext(bool is_secure_context) {
  is_secure_context_ = is_secure_context;
}

bool NullExecutionContext::IsSecureContext(String& error_message) const {
  if (!is_secure_context_)
    error_message = "A secure context is required";
  return is_secure_context_;
}

void NullExecutionContext::SetUpSecurityContext() {
  ContentSecurityPolicy* policy = ContentSecurityPolicy::Create();
  SecurityContext::SetSecurityOrigin(SecurityOrigin::Create(url_));
  policy->BindToExecutionContext(this);
  SecurityContext::SetContentSecurityPolicy(policy);
}

scoped_refptr<base::SingleThreadTaskRunner> NullExecutionContext::GetTaskRunner(
    TaskType) {
  return Platform::Current()->CurrentThread()->GetWebTaskRunner();
}

}  // namespace blink
