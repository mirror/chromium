// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/input/legacy_ipc_widget_input_handler.h"

#include "base/strings/utf_string_conversions.h"
#include "content/browser/renderer_host/input/legacy_input_router_impl.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/common/input_messages.h"

namespace content {

namespace {
std::vector<blink::WebCompositionUnderline> ConvertToBlinkUnderline(
    const std::vector<ui::CompositionUnderline>& ui_underlines) {
  std::vector<blink::WebCompositionUnderline> underlines;
  for (const auto& underline : ui_underlines) {
    blink::WebCompositionUnderline blink_underline(
        underline.start_offset, underline.end_offset, underline.color,
        underline.thick, underline.background_color);
    underlines.push_back(blink_underline);
  }
  return underlines;
}

}  // namespace

LegacyIPCWidgetInputHandler::LegacyIPCWidgetInputHandler(
    RenderWidgetHostImpl* widget_host,
    int routing_id)
    : widget_host_(widget_host), routing_id_(routing_id) {
  DCHECK(widget_host);
}

LegacyIPCWidgetInputHandler::~LegacyIPCWidgetInputHandler() {}

void LegacyIPCWidgetInputHandler::SetFocus(bool focused) {
  SendInput(base::MakeUnique<InputMsg_SetFocus>(routing_id_, focused));
}

void LegacyIPCWidgetInputHandler::MouseCaptureLost() {}

void LegacyIPCWidgetInputHandler::SetEditCommandsForNextKeyEvent(
    const std::vector<EditCommand>& commands) {
  SendInput(base::MakeUnique<InputMsg_SetEditCommandsForNextKeyEvent>(
      routing_id_, commands));
}

void LegacyIPCWidgetInputHandler::CursorVisibilityChanged(bool visible) {
  SendInput(
      base::MakeUnique<InputMsg_CursorVisibilityChange>(routing_id_, visible));
}

void LegacyIPCWidgetInputHandler::ImeSetComposition(
    const base::string16& text,
    const std::vector<ui::CompositionUnderline>& ui_underlines,
    const gfx::Range& range,
    int32_t start,
    int32_t end) {
  std::vector<blink::WebCompositionUnderline> underlines =
      ConvertToBlinkUnderline(ui_underlines);
  SendInput(base::MakeUnique<InputMsg_ImeSetComposition>(
      routing_id_, text, underlines, range, start, end));
}

void LegacyIPCWidgetInputHandler::ImeCommitText(
    const base::string16& text,
    const std::vector<ui::CompositionUnderline>& ui_underlines,
    const gfx::Range& range,
    int32_t relative_cursor_position) {
  std::vector<blink::WebCompositionUnderline> underlines =
      ConvertToBlinkUnderline(ui_underlines);
  SendInput(base::MakeUnique<InputMsg_ImeCommitText>(
      routing_id_, text, underlines, range, relative_cursor_position));
}

void LegacyIPCWidgetInputHandler::ImeFinishComposingText(bool keep_selection) {
  SendInput(base::MakeUnique<InputMsg_ImeFinishComposingText>(routing_id_,
                                                              keep_selection));
}
void LegacyIPCWidgetInputHandler::RequestTextInputStateUpdate() {
#if defined(OS_ANDROID)
  SendInput(
      base::MakeUnique<InputMsg_RequestTextInputStateUpdate>(routing_id_));
#endif
}

void LegacyIPCWidgetInputHandler::RequestCompositionUpdates(
    bool immediate_request,
    bool monitor_request) {
  SendInput(base::MakeUnique<InputMsg_RequestCompositionUpdates>(
      routing_id_, immediate_request, monitor_request));
}

void LegacyIPCWidgetInputHandler::SendInput(
    std::unique_ptr<IPC::Message> message) {
  static_cast<LegacyInputRouterImpl*>(widget_host_->input_router())
      ->SendInput(std::move(message));
}

void LegacyIPCWidgetInputHandler::DispatchEvent(
    std::unique_ptr<content::InputEvent> event,
    DispatchEventCallback callback) {
  NOTREACHED();
}

void LegacyIPCWidgetInputHandler::DispatchNonBlockingEvent(
    std::unique_ptr<content::InputEvent> event) {
  NOTREACHED();
}

}  // namespace content
