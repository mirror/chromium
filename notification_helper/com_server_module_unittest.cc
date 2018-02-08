// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "notification_helper/com_server_module.h"

#include <wrl/client.h>

#include "base/win/scoped_winrt_initializer.h"
#include "base/win/windows_version.h"
#include "chrome/install_static/install_util.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(ComServerModule, EventSignalTest) {
  // WinRT works on Windows 8 or above.
  if (base::win::GetVersion() < base::win::VERSION_WIN8)
    return;

  // Initialize WinRT.
  base::win::ScopedWinrtInitializer winrt_initializer;
  ASSERT_TRUE(winrt_initializer.Succeeded());

  // Run the COM server in this test EXE, and register the COM class object to
  // the server module.
  notification_helper::ComServerModule server_module;
  ASSERT_HRESULT_SUCCEEDED(server_module.RegisterClassObjects());

  // The waitable event starts unsignaled.
  ASSERT_FALSE(server_module.IsEventSignaled());

  // Create a class instance.
  Microsoft::WRL::ComPtr<IUnknown> notification_activator;
  ASSERT_HRESULT_SUCCEEDED(::CoCreateInstance(
      install_static::GetToastActivatorClsid(), nullptr, CLSCTX_LOCAL_SERVER,
      IID_PPV_ARGS(&notification_activator)));

  // The module now holds a reference of the instance object, hence the waitable
  // event remains unsignaled.
  ASSERT_FALSE(server_module.IsEventSignaled());

  // Release the instance object. Now that the last (and the only) instance
  // object of the module is released, the event is signaled via
  // ComServerModule::SignalObjectReleaseEvent.
  notification_activator.Reset();
  ASSERT_TRUE(server_module.IsEventSignaled());

  server_module.UnregisterClassObjects();
}
