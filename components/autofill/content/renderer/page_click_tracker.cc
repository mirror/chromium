// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/renderer/page_click_tracker.h"

#include "base/command_line.h"
#include "components/autofill/content/renderer/autofill_element.h"
#include "components/autofill/content/renderer/form_autofill_util.h"
#include "components/autofill/content/renderer/page_click_listener.h"
#include "components/autofill/core/common/autofill_util.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "third_party/WebKit/public/platform/WebPoint.h"
#include "third_party/WebKit/public/platform/WebSize.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebFormControlElement.h"
#include "third_party/WebKit/public/web/WebHitTestResult.h"
#include "third_party/WebKit/public/web/WebInputElement.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebUserGestureIndicator.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "third_party/WebKit/webagents/agent.h"
#include "third_party/WebKit/webagents/frame.h"
#include "third_party/WebKit/webagents/html_input_element.h"
#include "third_party/WebKit/webagents/html_text_area_element.h"
#include "third_party/WebKit/webagents/node.h"

using blink::WebElement;
using blink::WebFormControlElement;
using blink::WebInputElement;
using blink::WebNode;
using blink::WebPoint;
using blink::WebSize;
using blink::WebUserGestureIndicator;

using webagents::Element;
using webagents::HTMLInputElement;
using webagents::HTMLTextAreaElement;
using webagents::Node;

namespace autofill {

namespace {

// Casts |element| to a WebFormControlElement, but only if it's a text field.
// Returns an empty (IsNull()) wrapper otherwise.
const WebFormControlElement GetTextFormControlElement(
    const WebElement& element) {
  if (!element.IsFormControlElement())
    return WebFormControlElement();
  if (form_util::IsTextInput(blink::ToWebInputElement(&element)) ||
      element.HasHTMLTagName("textarea"))
    return element.ToConst<WebFormControlElement>();
  return WebFormControlElement();
}

}  // namespace

PageClickTracker::PageClickTracker(content::RenderFrame* render_frame,
                                   PageClickListener* listener)
    : focused_node_was_last_clicked_(false),
      was_focused_before_now_(false),
      listener_(listener),
      render_frame_(render_frame),
      agent_(nullptr) {}

PageClickTracker::PageClickTracker(webagents::Agent* agent,
                                   PageClickListenerWebagent* listener)
    : focused_node_was_last_clicked_(false),
      was_focused_before_now_(false),
      listener_webagent_(listener),
      render_frame_(nullptr),
      agent_(agent) {}

PageClickTracker::~PageClickTracker() {
}

void PageClickTracker::FocusedNodeChanged(const WebNode& node) {
  was_focused_before_now_ = false;

  if (IsKeyboardAccessoryEnabled() &&
      WebUserGestureIndicator::IsProcessingUserGesture()) {
    focused_node_was_last_clicked_ = true;
    DoFocusChangeComplete();
  }
}

void PageClickTracker::DidCompleteFocusChangeInFrame() {
  if (IsKeyboardAccessoryEnabled())
    return;

  DoFocusChangeComplete();
}

// TODO: Agent should register to receive events.
void PageClickTracker::DidReceiveLeftMouseDownOrGestureTapInNode(
    const blink::WebNode& node) {
  DCHECK(!node.IsNull());
  focused_node_was_last_clicked_ = node.Focused();

  if (IsKeyboardAccessoryEnabled())
    DoFocusChangeComplete();
}

// TODO: Agent should register to receive events.
void PageClickTracker::DidReceiveLeftMouseDownOrGestureTapInNodeWebagents(
    const Node& node) {
  focused_node_was_last_clicked_ = node.NotStandardIsFocused();

  if (IsKeyboardAccessoryEnabled())
    DoFocusChangeComplete();
}

void PageClickTracker::DoFocusChangeComplete() {
  if (listener_webagent_) {
    base::Optional<Element> focused_element =
        GetAgent()->GetFrame().GetDocument().activeElement();
    if (!focused_node_was_last_clicked_ || !focused_element)
      return;

    // Only fire for text-input or textarea.
    if ((focused_element->IsHTMLInputElement() &&
         focused_element->ToHTMLInputElement().type() == "text") ||
        focused_element->IsHTMLTextAreaElement()) {
      base::Optional<AutofillElement> autofill_element =
          AutofillElement::FromElement(*focused_element);
      DCHECK(autofill_element);
      listener_webagent_->FormControlElementClickedWebagents(
          *autofill_element, was_focused_before_now_);
    }
  } else {
    WebElement focused_element =
        render_frame()->GetWebFrame()->GetDocument().FocusedElement();
    if (focused_node_was_last_clicked_ && !focused_element.IsNull()) {
      const WebFormControlElement control =
          GetTextFormControlElement(focused_element);
      listener_->FormControlElementClicked(control, was_focused_before_now_);
    }
  }

  was_focused_before_now_ = true;
  focused_node_was_last_clicked_ = false;
}

}  // namespace autofill
