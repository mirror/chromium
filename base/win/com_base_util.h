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
BASE_EXPORT class ComBaseUtil {
 public:
  // A utility method for callers that need to know upfront if any of the
  // functions required can not be loaded dynamically.
  BASE_EXPORT static bool PreloadFunctions(
      const std::vector<std::string>& functions);

  BASE_EXPORT static HRESULT RoGetActivationFactory(HSTRING class_id,
                                                    const IID& iid,
                                                    void** out_factory);

  BASE_EXPORT static HRESULT RoActivateInstance(HSTRING class_id,
                                                IInspectable** instance);

  BASE_EXPORT static HRESULT WindowsCreateString(const base::char16* src,
                                                 uint32_t len,
                                                 HSTRING* out_hstr);

  BASE_EXPORT static HRESULT WindowsDeleteString(HSTRING hstr);

  BASE_EXPORT static HRESULT WindowsCreateStringReference(
      const base::char16* src,
      uint32_t len,
      HSTRING_HEADER* out_header,
      HSTRING* out_hstr);

  BASE_EXPORT static const base::char16* WindowsGetStringRawBuffer(
      HSTRING hstr,
      uint32_t* out_len);

 private:
  ComBaseUtil() = default;
  ~ComBaseUtil() = default;

  FRIEND_TEST_ALL_PREFIXES(ComBaseUtilTest, PreloadFunctionInvalidData);
  FRIEND_TEST_ALL_PREFIXES(ComBaseUtilTest, PreloadFunction);
  FRIEND_TEST_ALL_PREFIXES(ComBaseUtilTest,
                           WindowsCreateStringReference_NoPreload);
  FRIEND_TEST_ALL_PREFIXES(ComBaseUtilTest,
                           WindowsCreateAndDeleteString_NoPreload);

  static HMODULE EnsureLibraryLoaded();
  BASE_EXPORT static HMODULE GetLibraryForTesting();

  static bool PreloadRoGetActivationFactory();
  static bool PreloadRoActivateInstance();
  static bool PreloadWindowsCreateString();
  static bool PreloadWindowsDeleteString();
  static bool PreloadWindowsCreateStringReference();
  static bool PreloadWindowsGetStringRawBuffer();

  static decltype(&::RoGetActivationFactory) get_factory_func_;
  static decltype(&::RoActivateInstance) activate_instance_func_;
  static decltype(&::WindowsCreateString) create_string_func_;
  static decltype(&::WindowsDeleteString) delete_string_func_;
  static decltype(&::WindowsCreateStringReference) create_string_ref_func_;
  static decltype(&::WindowsGetStringRawBuffer) get_string_raw_buffer_func_;

  static HMODULE combase_dll_;
};

}  // namespace win
}  // namespace base

#endif  // BASE_WIN_COM_BASE_UTIL_H_
