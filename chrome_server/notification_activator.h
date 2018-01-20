// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVER_NOTIFICATION_ACTIVATOR_H_
#define CHROME_SERVER_NOTIFICATION_ACTIVATOR_H_

#include "windows.h"

#include <NotificationActivationCallback.h>
#include <wrl.h>

#include <ShObjIdl.h>
#include <WinInet.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>

#include "base/logging.h"
#include "chrome/install_static/install_util.h"

namespace mswr = Microsoft::WRL;

EXTERN_C const GUID CLSID_NotificationActivator;

// A Win32 component that participates with Action Center will need to create a
// COM component that exposes the INotificationActivationCallback interface.

class ATL_NO_VTABLE DECLSPEC_UUID("071BB5F2-85A4-424F-BFE7-5F1609BE4C2C")
    NotificationActivator
    : public CComObjectRootEx<CComSingleThreadModel>,
      public CComCoClass<NotificationActivator, &CLSID_NotificationActivator>,
      public mswr::RuntimeClass<mswr::RuntimeClassFlags<mswr::ClassicCom>,
                                INotificationActivationCallback> {
 public:
  IFACEMETHODIMP Activate(LPCWSTR /* appUserModelId */,
                          LPCWSTR /* invokedArgs */,
                          const NOTIFICATION_USER_INPUT_DATA* /* data */,
                          ULONG /* count */) override;

 public:
  NotificationActivator();
  ~NotificationActivator() override;
};

#endif  // CHROME_SERVER_NOTIFICATION_ACTIVATOR_H_
