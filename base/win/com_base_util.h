// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_WIN_COM_BASE_UTIL_H_
#define BASE_WIN_COM_BASE_UTIL_H_

#include <Inspectable.h>
#include <Roapi.h>
#include <Winstring.h>

#include <vector>

#include "base/gtest_prod_util.h"
#include "base/strings/string16.h"

namespace base {
namespace win {

// Provides access to functions in combase.dll which may not be available on
// Windows 7. Loads functions dynamically at runtime to prevent library
// dependencies.
namespace ComBaseUtil {

BASE_EXPORT HMODULE EnsureLibraryLoaded();
BASE_EXPORT bool PreloadRoGetActivationFactory();
BASE_EXPORT bool PreloadRoActivateInstance();
BASE_EXPORT bool PreloadWindowsCreateString();
BASE_EXPORT bool PreloadWindowsDeleteString();
BASE_EXPORT bool PreloadWindowsGetStringRawBuffer();

BASE_EXPORT HRESULT RoGetActivationFactory(HSTRING class_id,
                                           const IID& iid,
                                           void** out_factory);

BASE_EXPORT HRESULT RoActivateInstance(HSTRING class_id,
                                       IInspectable** instance);

BASE_EXPORT HRESULT WindowsCreateString(const base::char16* src,
                                        uint32_t len,
                                        HSTRING* out_hstr);

BASE_EXPORT HRESULT WindowsDeleteString(HSTRING hstr);

BASE_EXPORT const base::char16* WindowsGetStringRawBuffer(HSTRING hstr,
                                                          uint32_t* out_len);

BASE_EXPORT HMODULE GetLibraryForTesting();

}  // namespace ComBaseUtil
}  // namespace win
}  // namespace base

#endif  // BASE_WIN_COM_BASE_UTIL_H_
