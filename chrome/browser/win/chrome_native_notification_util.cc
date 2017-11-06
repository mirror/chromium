// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_native_notification_util.h"

HRESULT STDMETHODCALLTYPE NotificationActivatorChrome::Activate(
    _In_ LPCWSTR /*appUserModelId*/,
    _In_ LPCWSTR invokedArgs,
    /*_In_reads_(dataCount)*/ const NOTIFICATION_USER_INPUT_DATA* /*data*/,
    ULONG /*dataCount*/) {
  // TODO(chengx): Implement.
  return S_OK;
}
