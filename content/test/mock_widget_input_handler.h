// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_TEST_MOCK_WIDGET_INPUT_HANDLER_H_
#define CONTENT_TEST_MOCK_WIDGET_INPUT_HANDLER_H_

#include <stddef.h>

#include <memory>
#include <utility>

#include "content/common/input/input_handler.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace content {

class MockWidgetInputHandler : public mojom::WidgetInputHandler {
 public:
  MockWidgetInputHandler();
  MockWidgetInputHandler(mojom::WidgetInputHandlerRequest request,
                         mojom::WidgetInputHandlerHostPtr host);
  ~MockWidgetInputHandler() override;

  class DispatchedEditCommandMessage;
  class DispatchedEventMessage;
  class DispatchedIMEMessage;

  class DispatchedMessage {
   public:
    DispatchedMessage(const std::string& name);
    virtual ~DispatchedMessage();

    virtual DispatchedEditCommandMessage* ToEditCommand();
    virtual DispatchedEventMessage* ToEvent();
    virtual DispatchedIMEMessage* ToIME();

    std::string name_;
  };

  class DispatchedIMEMessage : public DispatchedMessage {
   public:
    DispatchedIMEMessage(const std::string& name,
                         const base::string16& text,
                         const std::vector<ui::ImeTextSpan>& ime_text_spans,
                         const gfx::Range& range,
                         int32_t start,
                         int32_t end);
    ~DispatchedIMEMessage() override;

    DispatchedIMEMessage* ToIME() override;

    base::string16 text_;
    std::vector<ui::ImeTextSpan> text_spans_;
    gfx::Range range_;
    int32_t start_;
    int32_t end_;
  };

  class DispatchedEditCommandMessage : public DispatchedMessage {
   public:
    DispatchedEditCommandMessage(
        const std::vector<content::EditCommand>& commands);
    ~DispatchedEditCommandMessage() override;

    DispatchedEditCommandMessage* ToEditCommand() override;

    std::vector<content::EditCommand> commands_;
  };

  class DispatchedEventMessage : public DispatchedMessage {
   public:
    DispatchedEventMessage(std::unique_ptr<content::InputEvent> event,
                           DispatchEventCallback callback);
    ~DispatchedEventMessage() override;

    DispatchedEventMessage* ToEvent() override;
    void CallCallback(InputEventAckState state);

    std::unique_ptr<content::InputEvent> event_;
    DispatchEventCallback callback_;
  };

  // mojom::WidgetInputHandler override.
  void SetFocus(bool focused) override;
  void MouseCaptureLost() override;
  void SetEditCommandsForNextKeyEvent(
      const std::vector<content::EditCommand>& commands) override;
  void CursorVisibilityChanged(bool visible) override;
  void ImeSetComposition(const base::string16& text,
                         const std::vector<ui::ImeTextSpan>& ime_text_spans,
                         const gfx::Range& range,
                         int32_t start,
                         int32_t end) override;
  void ImeCommitText(const base::string16& text,
                     const std::vector<ui::ImeTextSpan>& ime_text_spans,
                     const gfx::Range& range,
                     int32_t relative_cursor_position) override;
  void ImeFinishComposingText(bool keep_selection) override;
  void RequestTextInputStateUpdate() override;
  void RequestCompositionUpdates(bool immediate_request,
                                 bool monitor_request) override;

  void DispatchEvent(std::unique_ptr<content::InputEvent> event,
                     DispatchEventCallback callback) override;
  void DispatchNonBlockingEvent(
      std::unique_ptr<content::InputEvent> event) override;

  using MessageVector = std::vector<std::unique_ptr<DispatchedMessage>>;
  MessageVector GetAndResetDispatchedMessages();

 private:
  mojo::Binding<mojom::WidgetInputHandler> binding_;
  mojom::WidgetInputHandlerHostPtr host_;
  MessageVector dispatched_messages_;
};

}  // namespace content

#endif  // CONTENT_TEST_MOCK_INPUT_ACK_HANDLER_H_
