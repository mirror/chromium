// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/events/EventListener.h"
#include "core/html/HTMLElement.h"
#include "core/input/EventHandler.h"
#include "core/input/TouchEventManager.h"
#include "core/testing/sim/SimCompositor.h"
#include "core/testing/sim/SimDisplayItemList.h"
#include "core/testing/sim/SimRequest.h"
#include "core/testing/sim/SimTest.h"

namespace blink {

class TouchEventManagerTest : public SimTest {
 protected:
  EventHandler& EventHandler() {
    return GetDocument().GetFrame()->GetEventHandler();
  }
};

class CheckEventListenerCallback final : public EventListener {
 public:
  static CheckEventListenerCallback* Create() {
    return new CheckEventListenerCallback();
  }
  bool operator==(const EventListener& other) const override {
    return this == &other;
  }

  void handleEvent(ExecutionContext*, Event* event) override {
    event_received_ = true;
  }

  bool HasReceivedEvent() const { return event_received_; }

 private:
  CheckEventListenerCallback()
      : EventListener(EventListener::kCPPEventListenerType) {
    event_received_ = false;
  }
  bool event_received_;
};

TEST_F(TouchEventManagerTest, LostTouchDueToInnerIframeRemove) {
  WebView().Resize(WebSize(400, 400));
  SimRequest request("https://example.com/test.html", "text/html");
  LoadURL("https://example.com/test.html");
  request.Complete(R"HTML(
    <body style='padding: 0px; width: 400px; height: 400px;'>
    <iframe id='target' style='width: 200px; height: 200px;'></iframe>
    </body>
  )HTML");
  CheckEventListenerCallback* callback = CheckEventListenerCallback::Create();
  GetDocument().body()->addEventListener(EventTypeNames::touchstart, callback);

  WebPointerEvent event(
      WebPointerProperties(1, WebPointerProperties::PointerType::kTouch,
                           WebPointerProperties::Button::kLeft,
                           WebFloatPoint(100, 100), WebFloatPoint(100, 100)),
      WebInputEvent::kPointerDown, 1, 1);
  event.SetFrameScale(1);
  EventHandler().HandlePointerEvent(event, Vector<WebPointerEvent>());
  EventHandler().DispatchPendingTouchEvents();

  GetDocument().getElementById("target")->remove();

  event.SetType(WebInputEvent::kPointerUp);
  EventHandler().HandlePointerEvent(event, Vector<WebPointerEvent>());
  EventHandler().DispatchPendingTouchEvents();

  event.SetType(WebInputEvent::kPointerDown);
  EventHandler().HandlePointerEvent(event, Vector<WebPointerEvent>());
  EventHandler().DispatchPendingTouchEvents();

  ASSERT_TRUE(callback->HasReceivedEvent());
}

}  // namespace blink
