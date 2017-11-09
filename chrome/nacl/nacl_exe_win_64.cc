// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "build/build_config.h"
#include "chrome/app/chrome_crash_reporter_client_win.h"
#include "chrome/install_static/product_install_details.h"
#include "components/nacl/loader/nacl_helper_win_64.h"
#include "content/public/common/content_switches.h"

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, wchar_t*, int) {
#if defined(OS_WIN)
  install_static::InitializeProductDetailsForPrimaryModule();
#endif

  base::AtExitManager exit_manager;
  base::CommandLine::Init(0, NULL);

  ChromeCrashReporterClient::InitializeCrashReportingForProcess();

  return nacl::NaClWin64Main();
}
