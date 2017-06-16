// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/media_galleries/media_galleries_dialog_controller_mock.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "chrome/browser/ui/views/extensions/media_galleries_dialog_views.h"
#include "chrome/browser/ui/views/extensions/media_gallery_checkbox_view.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/storage_monitor/storage_info.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/common/referrer.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"
#include "ui/views/controls/button/checkbox.h"
#include "url/gurl.h"
#include "url/url_constants.h"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Mock;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

MediaGalleryPrefInfo MakePrefInfoForTesting(MediaGalleryPrefId id) {
  MediaGalleryPrefInfo gallery;
  gallery.pref_id = id;
  gallery.device_id = storage_monitor::StorageInfo::MakeDeviceId(
      storage_monitor::StorageInfo::FIXED_MASS_STORAGE,
      base::Uint64ToString(id));
  gallery.display_name = base::ASCIIToUTF16("Display Name");
  return gallery;
}

}  // namespace

class MediaGalleriesInteractiveDialogTest : public DialogBrowserTest {
 public:
  MediaGalleriesInteractiveDialogTest() {}
  ~MediaGalleriesInteractiveDialogTest() override {}

  //  void TearDown() override { Mock::VerifyAndClearExpectations(&controller_);
  //  }

  void PreRunTestOnMainThread() override {
    DialogBrowserTest::PreRunTestOnMainThread();
    const GURL about_blank(url::kAboutBlankURL);
    content::WebContents* content = browser()->OpenURL(content::OpenURLParams(
        about_blank, content::Referrer(), WindowOpenDisposition::CURRENT_TAB,
        ui::PAGE_TRANSITION_TYPED, true));
    EXPECT_CALL(*controller(), WebContents()).WillRepeatedly(Return(content));
    content::TestNavigationManager manager(content, about_blank);
    manager.WaitForNavigationFinished();
  }

  NiceMock<MediaGalleriesDialogControllerMock>* controller() {
    return &controller_;
  }

  void SetupHeaders() {
    std::vector<base::string16> headers;
    headers.push_back(base::string16());
    headers.push_back(base::ASCIIToUTF16("header2"));
    ON_CALL(controller_, GetSectionHeaders()).WillByDefault(Return(headers));
    EXPECT_CALL(controller_, GetSectionEntries(_)).Times(AnyNumber());
  }

  void ShowDialog(const std::string& name) override { return DisplayDialog(); }

  void DisplayDialog() {
    SetupHeaders();
    MediaGalleriesDialogController::Entries attached_permissions;
    attached_permissions.push_back(
        MediaGalleriesDialogController::Entry(MakePrefInfoForTesting(1), true));
    attached_permissions.push_back(MediaGalleriesDialogController::Entry(
        MakePrefInfoForTesting(2), false));
    EXPECT_CALL(*controller(), GetSectionEntries(0))
        .WillRepeatedly(Return(attached_permissions));

    dialog_ = base::MakeUnique<MediaGalleriesDialogViews>(controller());
  }

 private:
  NiceMock<MediaGalleriesDialogControllerMock> controller_;
  std::unique_ptr<MediaGalleriesDialogViews> dialog_;

  DISALLOW_COPY_AND_ASSIGN(MediaGalleriesInteractiveDialogTest);
};

IN_PROC_BROWSER_TEST_F(MediaGalleriesInteractiveDialogTest,
                       InvokeDialog_DisplayDialog) {
  RunDialog();
}
