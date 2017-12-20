// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_DWRITE_FONT_PROXY_MESSAGES_H_
#define CONTENT_COMMON_DWRITE_FONT_PROXY_MESSAGES_H_

#include <stdint.h>

#include <utility>
#include <vector>

#include "base/strings/string16.h"
#include "content/common/content_export.h"
#include "ipc/ipc_message_macros.h"
#include "ipc/ipc_platform_file.h"

#undef IPC_MESSAGE_EXPORT
#define IPC_MESSAGE_EXPORT CONTENT_EXPORT
#define IPC_MESSAGE_START DWriteFontProxyMsgStart

#endif  // CONTENT_COMMON_DWRITE_FONT_PROXY_MESSAGES_H_
