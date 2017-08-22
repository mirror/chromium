// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SYS_INTERNALS_SYS_INTERNALS_MESSAGE_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_SYS_INTERNALS_SYS_INTERNALS_MESSAGE_HANDLER_H_

#include "base/macros.h"
#include "base/process/process_metrics.h"
#include "base/values.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace base {
struct CpuInfo {
  int kernel;
  int user;
  int idle;
  int total;
};
}  // namespace base

class SysInternalsMessageHandler : public content::WebUIMessageHandler {
 public:
  SysInternalsMessageHandler();
  ~SysInternalsMessageHandler() override;

  void RegisterMessages() override;
  static const uint32_t COUNTER_MAX = 0x7FFFFFFFu;

 private:
  std::vector<base::CpuInfo> cpu_infos_;
  base::SystemMemoryInfoKB mem_info_;
  base::SwapInfo swap_info_;

  void ConsoleWarn(const char* message);
  void GetSysInfo(const base::ListValue* args);
  void SetValueAndReplySysInfo(std::unique_ptr<base::Value> time_tick,
                               bool is_console_log);
  void UpdateInfoStructures();

  DISALLOW_COPY_AND_ASSIGN(SysInternalsMessageHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_SYS_INTERNALS_SYS_INTERNALS_MESSAGE_HANDLER_H_
