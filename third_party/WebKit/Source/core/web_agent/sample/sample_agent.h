// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEB_AGENT_SAMPLE_AGENT_H_
#define WEB_AGENT_SAMPLE_AGENT_H_

#include "core/web_agent/agent.h"

namespace web {
class SampleAgent : public Agent {
 public:
  explicit SampleAgent(Frame*);
};
}  // namespace web
#endif  // WEB_AGENT_SAMPLE_AGENT_H_
