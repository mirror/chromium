/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include "core/editing/SurroundingText.h"

// TODO(xiaochengh): Merge this file into core/editing/SurroundingText.cpp

#include "core/dom/Element.h"
#include "core/dom/Node.h"
#include "core/dom/Range.h"
#include "core/dom/Text.h"
#include "core/editing/EphemeralRange.h"
#include "core/editing/FrameSelection.h"
#include "core/editing/SurroundingText.h"
#include "core/editing/VisibleSelection.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/WebLocalFrameImpl.h"
#include "core/layout/LayoutObject.h"
#include "public/platform/WebPoint.h"
#include "public/web/WebHitTestResult.h"

namespace blink {

// static
WebSurroundingText* WebSurroundingText::CreateFromCurrentSelection(
    WebLocalFrame* frame,
    size_t max_length) {
  LocalFrame* web_frame = ToWebLocalFrameImpl(frame)->GetFrame();

  // TODO(editing-dev): The use of updateStyleAndLayoutIgnorePendingStylesheets
  // needs to be audited.  See http://crbug.com/590369 for more details.
  web_frame->GetDocument()->UpdateStyleAndLayoutIgnorePendingStylesheets();

  const EphemeralRange range = web_frame->Selection()
                                   .ComputeVisibleSelectionInDOMTree()
                                   .ToNormalizedEphemeralRange();
  return new SurroundingText(range, max_length);
}

WebString SurroundingText::TextContent() const {
  return Content();
}

size_t SurroundingText::StartOffsetInTextContent() const {
  return StartOffsetInContent();
}

size_t SurroundingText::EndOffsetInTextContent() const {
  return EndOffsetInContent();
}

}  // namespace blink
