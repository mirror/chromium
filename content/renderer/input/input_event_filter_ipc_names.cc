// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/input/input_event_filter.h"

#include <tuple>
#include <utility>

#include "base/auto_reset.h"
#include "base/bind.h"
#include "base/debug/crash_logging.h"
#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "content/common/input_messages.h"
#include "content/common/view_messages.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/renderer/input/input_handler_manager.h"
#include "content/renderer/render_thread_impl.h"
#include "ipc/ipc_listener.h"
#include "ipc/ipc_sender.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/blink/did_overscroll_params.h"
#include "ui/events/blink/web_input_event_traits.h"
#include "ui/gfx/geometry/vector2d_f.h"

#include "ipc/ipc_message_macros.h"

#include "ipc/ipc_message_null_macros.h"
#undef IPC_MESSAGE_DECL
#define IPC_MESSAGE_DECL(name, ...) \
  case name::ID:                    \
    return #name;

const char* GetInputMessageTypeName(const IPC::Message& message) {
  switch (message.type()) {
#undef CONTENT_COMMON_INPUT_MESSAGES_H_
#include "content/common/input_messages.h"
#ifndef CONTENT_COMMON_INPUT_MESSAGES_H_
#error "Failed to include content/common/input_messages.h"
#endif
    default:
      NOTREACHED() << "Invalid message type: " << message.type();
      break;
  };
  return "NonInputMsgType";
}
