// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SYS_INTERNALS_SYS_INTERNALS_MESSAGE_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_SYS_INTERNALS_SYS_INTERNALS_MESSAGE_HANDLER_H_

#include <stdint.h>
#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/process/process_metrics.h"
#include "base/task_scheduler/post_task.h"
#include "base/values.h"
#include "content/public/browser/web_ui_message_handler.h"

// The handler class for SysInternals page operations.
class SysInternalsMessageHandler : public content::WebUIMessageHandler {
 public:
  SysInternalsMessageHandler();
  ~SysInternalsMessageHandler() override;

  // content::WebUIMessageHandler methods:
  void RegisterMessages() override;

  // Due to the |base::Value| API only supports 32-bit signed integer,
  // we need to make sure every counter is less than the maximum value.
  static const uint32_t COUNTER_MAX = 0x7FFFFFFFu;

 private:
  // Handles the IO operations.
  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;

  // Shows warning message on Javascript console.
  void ConsoleWarn(const char* message);

  // Handle the Javascript message |getSysInfo|. The message is sent to get
  // system information.
  void HandleGetSysInfo(const base::ListValue* args);

  // Call the callback function to handle the returning data.
  // Data format:
  // {
  //   const.counterMax (Integer):  The maximum value of all counters.
  //   cpus (Array): [ cpuInfo... ]  The cpus information.
  //   memory (Object): The memory information.
  //   zram (Object): The zram information.
  // }
  // Due to the |base::Value| api only supports 32-bit signed integer,
  // values that may greater than 2^31 will be passed as a double. If the value
  // is a counter, we only use the last 31 bits and will pass it as a 32-bit
  // signed integer.
  void SetValueAndReplySysInfo(std::unique_ptr<base::Value> callback_id);

  DISALLOW_COPY_AND_ASSIGN(SysInternalsMessageHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_SYS_INTERNALS_SYS_INTERNALS_MESSAGE_HANDLER_H_
