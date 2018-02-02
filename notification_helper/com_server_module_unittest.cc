// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "notification_helper/com_server_module.h"

#include <windows.h>

#include <NotificationActivationCallback.h>
#include <wrl/client.h>

#include "base/win/scoped_winrt_initializer.h"
#include "chrome/install_static/install_util.h"
#include "chrome/install_static/product_install_details.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(ComServerModule, EventSignalTest) {
  install_static::InitializeProductDetailsForPrimaryModule();

  // Initialize WinRT.
  base::win::ScopedWinrtInitializer winrt_initializer;
  DCHECK(winrt_initializer.Succeeded());

  // Run the COM server in this test EXE, register the COM class object to the
  // server module.
  notification_helper::ComServerModule server_module;
  server_module.RegisterClassObjects();

  // The waitable event starts unsignaled.
  EXPECT_FALSE(server_module.IsEventSignaled());

  // Create a class instance.
  /*Microsoft::WRL::ComPtr<IUnknown> notification_activator;*/
  INotificationActivationCallback* notification_activator;
  HRESULT hr = ::CoCreateInstance(install_static::GetToastActivatorClsid(),
                                  nullptr, CLSCTX_LOCAL_SERVER,
                                  IID_PPV_ARGS(&notification_activator));
  ASSERT_HRESULT_SUCCEEDED(hr);

  // The module now holds a reference of the instance object, hence the waitable
  // event is still unsignaled.
  EXPECT_FALSE(server_module.IsEventSignaled());

  // Release the instance object. Now that the last (and the only) instance
  // object of the module is released, the event is signaled via
  // ComServerModule::SignalObjectReleaseEvent.
  notification_activator->Release();
  EXPECT_TRUE(server_module.IsEventSignaled());

  server_module.UnregisterClassObjects();
}
