// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/inspector/InspectorPasswordManagerAgent.h"

#include "platform/wtf/CurrentTime.h"

#include <stdio.h>      /* fopen, fputs, fclose, stderr */
#include <stdlib.h>     /* abort, NULL */

namespace blink {

  using protocol::Response;

  InspectorPasswordManagerAgent::InspectorPasswordManagerAgent() {
    // enable();
    // printf("varkor: InspectorPasswordManagerAgent::enable\n");
    // instrumenting_agents_->addInspectorPasswordManagerAgent(this);
  }

  // void InspectorPasswordManagerAgent::Restore() {
  //   // if (state_->booleanProperty(PageAgentState::kPageAgentEnabled, false))
  //     enable();
  // }

  Response InspectorPasswordManagerAgent::enable() {
    // enabled_ = true;
    printf("varkor: InspectorPasswordManagerAgent::enable\n");
    instrumenting_agents_->addInspectorPasswordManagerAgent(this);
    return Response::OK();
  }

  void InspectorPasswordManagerAgent::TrivialPasswordManagerEventFired(LocalFrame* frame) {
    printf("varkor: InspectorPasswordManagerAgent::TrivialPasswordManagerEventFired\n");
    GetFrontend()->trivialPasswordManagerEvent(MonotonicallyIncreasingTime());
  }

}  // namespace blink
