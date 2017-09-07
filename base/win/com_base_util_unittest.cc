// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/win/com_base_util.h"

#include <windows.ui.notifications.h>
#include <wrl/client.h>
#include <wrl/wrappers/corewrappers.h>

#include "base/test/gtest_util.h"
#include "base/win/scoped_hstring.h"
#include "base/win/windows_version.h"

namespace base {
namespace win {
namespace ComBaseUtil {

class ComBaseUtilTest : public testing::Test {
 public:
  ComBaseUtilTest() : expect_success_(false) {
    if (base::win::GetVersion() >= base::win::VERSION_WIN10_RS1) {
      expect_success_ = true;
      CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    }
  }

  ~ComBaseUtilTest() {
    if (expect_success_)
      CoUninitialize();
  }

  bool expectation() { return expect_success_; }

  void FreeComBaseModule() {
    if (expect_success_)
      FreeLibrary(ComBaseUtil::GetComBaseModuleHandle());
  }

 private:
  bool expect_success_;
};

TEST_F(ComBaseUtilTest, PreloadFunctions) {
  decltype(&::RoGetActivationFactory) func1 = PreloadRoGetActivationFactory();
  EXPECT_TRUE(expectation() == (func1 != nullptr));
  FreeComBaseModule();

  decltype(&::RoActivateInstance) func2 = PreloadRoActivateInstance();
  EXPECT_TRUE(expectation() == (func2 != nullptr));
  FreeComBaseModule();
}

TEST_F(ComBaseUtilTest, RoActivateInstance_NoPreload) {
  ScopedHString ref_class_name(RuntimeClass_Windows_Data_Xml_Dom_XmlDocument);
  Microsoft::WRL::ComPtr<IInspectable> inspectable;
  HRESULT hr =
      ComBaseUtil::RoActivateInstance(ref_class_name.get(), &inspectable);
  EXPECT_EQ(expectation(), SUCCEEDED(hr)) << "Error" << hr;
  FreeComBaseModule();
}

}  // namespace ComBaseUtil
}  // namespace win
}  // namespace base
