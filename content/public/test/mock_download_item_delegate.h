// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_MOCK_DOWNLOAD_ITEM_DELEGATE_H_
#define CONTENT_PUBLIC_TEST_MOCK_DOWNLOAD_ITEM_DELEGATE_H_

#include "content/public/browser/content_download_item_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

class BrowserContext;
class WebContents;

class MockDownloadItemDelegate : public content::ContentDownloadItemDelegate {
 public:
  MockDownloadItemDelegate();
  ~MockDownloadItemDelegate() override;

  MOCK_CONST_METHOD0(GetBrowserContext, BrowserContext*());
  MOCK_CONST_METHOD0(GetWebContents, WebContents*());
};

}  // namespace content

#endif  // CONTENT_PUBLIC_TEST_MOCK_DOWNLOAD_ITEM_DELEGATE_H_
