// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/testing/sim/SimWebFrameClient.h"

#include "core/testing/sim/SimTest.h"
#include "public/web/WebConsoleMessage.h"
#include "public/web/WebPlugin.h"

namespace blink {

namespace {

class TestPlugin : public WebPlugin {
 public:
  // TODO(dcheng): WebPlugin should have a virtual dtor.
  virtual ~TestPlugin() {}

  // WebPlugin overrides:
  bool Initialize(WebPluginContainer* container) override {
    container_ = container;
    return true;
  }

  void Destroy() override { delete this; }

  WebPluginContainer* Container() const override { return container_; }

  void UpdateAllLifecyclePhases() override {}
  void Paint(WebCanvas*, const WebRect&) override {}

  void UpdateGeometry(const WebRect& window_rect,
                      const WebRect& clip_rect,
                      const WebRect& unobscured_rect,
                      bool is_visible) override {}

  void UpdateFocus(bool focused, WebFocusType) override {}

  void UpdateVisibility(bool) override {}

  WebInputEventResult HandleInputEvent(const WebCoalescedInputEvent&,
                                       WebCursorInfo&) override {
    return WebInputEventResult::kNotHandled;
  }

  void DidReceiveResponse(const WebURLResponse&) override {}
  void DidReceiveData(const char* data, int data_length) override {}
  void DidFinishLoading() override {}
  void DidFailLoading(const WebURLError&) override {}

 private:
  WebPluginContainer* container_;
};

}  // namespace

SimWebFrameClient::SimWebFrameClient(SimTest& test) : test_(&test) {}

WebPlugin* SimWebFrameClient::CreatePlugin(const WebPluginParams&) {
  return new TestPlugin;
}

void SimWebFrameClient::DidAddMessageToConsole(const WebConsoleMessage& message,
                                               const WebString& source_name,
                                               unsigned source_line,
                                               const WebString& stack_trace) {
  test_->AddConsoleMessage(message.text);
}

}  // namespace blink
