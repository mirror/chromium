// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "chrome/browser/ui/views/extensions/pwa_confirmation_view.h"
#include "chrome/common/web_application_info.h"
#include "components/constrained_window/constrained_window_views.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkColor.h"

namespace {

// Helper class to display the PWAConfirmationView dialog for testing.
class PWAConfirmationViewTest : public DialogBrowserTest {
 public:
  PWAConfirmationViewTest() {}

  void ShowDialog(const std::string& name) override {
    SkBitmap bmap;
    bmap.allocN32Pixels(kPWAConfirmationViewIconSize,
                        kPWAConfirmationViewIconSize, true);
    bmap.eraseColor(SK_ColorBLUE);
    WebApplicationInfo::IconInfo iconinf;
    iconinf.data = bmap;
    iconinf.width = kPWAConfirmationViewIconSize;
    iconinf.height = kPWAConfirmationViewIconSize;

    WebApplicationInfo wai;
    wai.icons.push_back(iconinf);
    if (name == "short_text") {
      wai.title = base::UTF8ToUTF16("Title");
      wai.app_url = GURL("https://www.example.com:9090/path");
    } else if (name == "long_text") {
      wai.title =
          base::UTF8ToUTF16("abcd\n1234567890123456789012345678901234567890");
      wai.app_url = GURL(
          "https://www"
          ".1234567890123456789012345678901234567890"
          ".1234567890123456789012345678901234567890"
          ".1234567890123456789012345678901234567890"
          ".1234567890123456789012345678901234567890"
          ".com:443/path");
    }
    constrained_window::CreateBrowserModalDialogViews(
        new PWAConfirmationView(
            wai, chrome::AppInstallationAcceptanceCallback(nullptr)),
        browser()->window()->GetNativeWindow())
        ->Show();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(PWAConfirmationViewTest);
};

IN_PROC_BROWSER_TEST_F(PWAConfirmationViewTest, InvokeDialog_default) {
  RunDialog();
}

IN_PROC_BROWSER_TEST_F(PWAConfirmationViewTest, InvokeDialog_short_text) {
  RunDialog();
}

IN_PROC_BROWSER_TEST_F(PWAConfirmationViewTest, InvokeDialog_long_text) {
  RunDialog();
}

}  // namespace
