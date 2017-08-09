// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEB_AGENT_DOM_DISTILLER_H_
#define WEB_AGENT_DOM_DISTILLER_H_

#include "core/web_agent/agent.h"

namespace web {
class DomDistillerAgent : public Agent {
 public:
  explicit DomDistillerAgent(Frame*);
  virtual void DidMeaningfulLayout(blink::WebMeaningfulLayout) override;
};
}  // namespace web
#endif  // WEB_AGENT_DOM_DISTILLER_H_
