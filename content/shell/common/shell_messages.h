// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Multiply-included file, no traditional include guard.
#include <string>
#include <vector>

#include "base/values.h"
#include "content/public/common/common_param_traits.h"
#include "content/public/common/page_state.h"
#include "content/shell/common/leak_detection_result.h"
#include "ipc/ipc_message_macros.h"
#include "ipc/ipc_platform_file.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/ipc/gfx_param_traits.h"
#include "ui/gfx/ipc/skia/gfx_skia_param_traits.h"

#define IPC_MESSAGE_START ShellMsgStart

// Tells the renderer to reset all test runners.
IPC_MESSAGE_ROUTED0(ShellViewMsg_Reset)

// Sets the path to the WebKit checkout.
IPC_MESSAGE_CONTROL1(ShellViewMsg_SetWebKitSourceDir,
                     base::FilePath /* webkit source dir */)

// Tells the main window that a secondary renderer in a different process asked
// to finish the test.
IPC_MESSAGE_ROUTED0(ShellViewMsg_TestFinishedInSecondaryRenderer)

IPC_MESSAGE_ROUTED0(ShellViewMsg_TryLeakDetection)

// Notifies BlinkTestRunner that the layout dump has completed
// (and that it can proceed with finishing up the test).
IPC_MESSAGE_ROUTED0(ShellViewMsg_LayoutDumpCompleted)

// Send a custom text dump of the WebContents to the render host.
IPC_MESSAGE_ROUTED1(ShellViewHostMsg_CustomTextDump, std::string /* dump */)

// Initiates a layout dump. The browser will generate a dump from the required
// renderers. ShellViewMsg_LayoutDumpCompleted will be sent once all the dumps
// have been generated.
IPC_MESSAGE_ROUTED2(ShellViewHostMsg_InitiateLayoutDump,
                    bool /*should_dump_recursively*/,
                    bool /*should_dump_history*/)

// Send an image dump of the WebContents to the render host.
IPC_MESSAGE_ROUTED2(ShellViewHostMsg_ImageDump,
                    std::string /* actual pixel hash */,
                    SkBitmap /* image */)

// Send an audio dump to the render host.
IPC_MESSAGE_ROUTED1(ShellViewHostMsg_AudioDump,
                    std::vector<unsigned char> /* audio data */)

IPC_MESSAGE_ROUTED0(ShellViewHostMsg_TestFinished)

IPC_MESSAGE_ROUTED0(ShellViewHostMsg_ResetDone)

// WebTestDelegate related.
IPC_MESSAGE_ROUTED1(ShellViewHostMsg_OverridePreferences,
                    content::WebPreferences /* preferences */)
IPC_MESSAGE_ROUTED1(ShellViewHostMsg_PrintMessage,
                    std::string /* message */)
IPC_MESSAGE_ROUTED1(ShellViewHostMsg_PrintMessageToStderr,
                    std::string /* message */)
IPC_MESSAGE_ROUTED0(ShellViewHostMsg_ClearDevToolsLocalStorage)
IPC_MESSAGE_ROUTED2(ShellViewHostMsg_ShowDevTools,
                    std::string /* settings */,
                    std::string /* frontend_url */)
IPC_MESSAGE_ROUTED2(ShellViewHostMsg_EvaluateInDevTools,
                    int /* call_id */,
                    std::string /* script */)
IPC_MESSAGE_ROUTED0(ShellViewHostMsg_CloseDevTools)
IPC_MESSAGE_ROUTED1(ShellViewHostMsg_GoToOffset,
                    int /* offset */)
IPC_MESSAGE_ROUTED0(ShellViewHostMsg_Reload)
IPC_MESSAGE_ROUTED2(ShellViewHostMsg_LoadURLForFrame,
                    GURL /* url */,
                    std::string /* frame_name */)
IPC_MESSAGE_ROUTED0(ShellViewHostMsg_CloseRemainingWindows)

IPC_STRUCT_TRAITS_BEGIN(content::LeakDetectionResult)
IPC_STRUCT_TRAITS_MEMBER(leaked)
IPC_STRUCT_TRAITS_MEMBER(detail)
IPC_STRUCT_TRAITS_END()

IPC_MESSAGE_ROUTED1(ShellViewHostMsg_LeakDetectionDone,
                    content::LeakDetectionResult /* result */)

IPC_MESSAGE_ROUTED1(ShellViewHostMsg_SetBluetoothManualChooser,
                    bool /* enable */)
IPC_MESSAGE_ROUTED0(ShellViewHostMsg_GetBluetoothManualChooserEvents)
IPC_MESSAGE_ROUTED1(ShellViewMsg_ReplyBluetoothManualChooserEvents,
                    std::vector<std::string> /* events */)
IPC_MESSAGE_ROUTED2(ShellViewHostMsg_SendBluetoothManualChooserEvent,
                    std::string /* event */,
                    std::string /* argument */)
IPC_MESSAGE_ROUTED1(ShellViewHostMsg_SetPopupBlockingEnabled,
                    bool /* block_popups */)
