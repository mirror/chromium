// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/input/EventHandler.h"
#include "core/input/TouchEventManager.h"
#include "core/testing/sim/SimCompositor.h"
#include "core/testing/sim/SimDisplayItemList.h"
#include "core/testing/sim/SimRequest.h"
#include "core/testing/sim/SimTest.h"

namespace blink {

class TouchEventManagerTest : public SimTest {
 protected:
  void SetUp() override;

  EventHandler& EventHandler() {
    return GetDocument().GetFrame()->GetEventHandler();
  }
};

void TouchEventManagerTest::SetUp() {
  SimTest::SetUp();
}

TEST_F(TouchEventManagerTest, LostTouchDueToInnerIframeReload) {
  WebView().Resize(WebSize(400, 400));
  SimRequest request("https://example.com/test.html", "text/html");
  LoadURL("https://example.com/test.html");
  request.Complete(
      "<body style='padding: 0px; width: 400px; height: 400px;'>"
      "<iframe id='target' style='width: 200px; height: 200px;'></iframe>"
      "<div id='log'></div>"
      "<script>"
      "function log(e) { document.getElementById('log').innerHTML = e.type; }"
      "document.body.addEventListener('touchstart', log);"
      "</script>"
      "</body>");

  WebTouchEvent event;
  WebTouchPoint point(
      WebPointerProperties(1, WebPointerProperties::PointerType::kTouch,
                           WebPointerProperties::Button::kLeft,
                           WebFloatPoint(100, 100), WebFloatPoint(100, 100)));

  point.state = WebTouchPoint::State::kStatePressed;
  event.touches[event.touches_length++] = point;
  EventHandler().HandleTouchEvent(event, Vector<WebTouchEvent>());

  GetDocument().getElementById("target")->remove();

  event.touches[0].state = WebTouchPoint::State::kStateReleased;
  EventHandler().HandleTouchEvent(event, Vector<WebTouchEvent>());

  event.touches[0].state = WebTouchPoint::State::kStatePressed;
  EventHandler().HandleTouchEvent(event, Vector<WebTouchEvent>());

  String s = GetDocument().getElementById("log")->innerText();
  ASSERT_EQ(s, "touchstart");
}

}  // namespace blink
