// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/web_agent/sample/sample_agent.h"

#include "core/web_agent/api/document.h"
#include "core/web_agent/api/element.h"

namespace web {

SampleAgent::SampleAgent(Frame* frame) : Agent(frame) {
  LOG(ERROR) << "SampleAgent=" << this << ", frame=" << frame
             << ", document=" << frame->GetDocument();
  Document* document = frame->GetDocument();
  if (!document)
    return;
  const Element* element = document->documentElement();
  if (!element)
    return;
}

}  // namespace web
