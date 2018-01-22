// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <windows.h>

#include "base/at_exit.h"
#include "base/win/scoped_com_initializer.h"
#include "base/win/scoped_winrt_initializer.h"
#include "chrome/install_static/install_util.h"
#include "chrome/install_static/product_install_details.h"
#include "chrome_server/notification_activator.h"
#include "chrome_server/resource.h"

extern "C" int WINAPI wWinMain(HINSTANCE /* instance */,
                               HINSTANCE /* previous_instance */,
                               LPWSTR /* command_line */,
                               int /* nShowCmd */) {
  install_static::InitializeProductDetailsForPrimaryModule();

  base::AtExitManager exit_manager;

  ::MessageBoxW(NULL, L"enter chrome_server winMain", L"", MB_OK);

  // Initialize COM.
  base::win::ScopedCOMInitializer com_initializer(
      base::win::ScopedCOMInitializer::kMTA);
  if (!com_initializer.Succeeded()) {
    LOG(ERROR) << "Failed initializing COM.";
    return -1;
  }

  // Initialize WinRT.
  base::win::ScopedWinrtInitializer winrt_initializer;
  if (!winrt_initializer.Succeeded()) {
    LOG(ERROR) << "Failed initializing win RT.";
    return -1;
  }

  // Register the NotificationActivator class object.
  ActivatorRegisterer activator_registerer;
  activator_registerer.Run();

  ::MessageBoxW(NULL, L"exit chrome_server winMain", L"", MB_OK);

  return 0;
}
