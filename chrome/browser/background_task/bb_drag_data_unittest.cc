// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef ENABLE_BACKGROUND_TASK

#include "base/ref_counted.h"
#include "base/string_util.h"
#include "chrome/browser/background_task/bb_drag_data.h"
#include "chrome/common/os_exchange_data.h"
#include "googleurl/src/gurl.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "webkit/glue/webdropdata.h"

typedef testing::Test BbDragDataTest;

TEST_F(BbDragDataTest, ParameterlessConstructor) {
  BbDragData drag_data;
  EXPECT_TRUE(drag_data.url().is_empty());
  EXPECT_TRUE(drag_data.title().empty());
}

// WebDropData with url but is_bb_drag==false.
TEST_F(BbDragDataTest, Constructor1) {
  std::string url_string("http://www.google.com/foo?bar=yes");
  std::wstring title(L"test_task");
  GURL url(url_string);

  WebDropData drop_data;
  drop_data.url = url;
  drop_data.url_title = title;

  BbDragData drag_data(drop_data);
  EXPECT_TRUE(drag_data.url().is_empty());
  EXPECT_TRUE(drag_data.title().empty());
}

// Now have the same WebDropData but with is_bb_drag==true.
TEST_F(BbDragDataTest, Constructor2) {
  std::string url_string("http://www.google.com/foo?bar=yes");
  std::wstring title(L"test_task");
  GURL url(url_string);

  WebDropData drop_data;
  drop_data.url = url;
  drop_data.url_title = title;
  drop_data.is_bb_drag = true;

  BbDragData drag_data(drop_data);
  EXPECT_EQ(drag_data.url(), url);
  EXPECT_EQ(drag_data.title(), title);
}

// Check the empty title replacement.
TEST_F(BbDragDataTest, Constructor3) {
  std::string url_string("http://www.google.com/foo?bar=yes");
  GURL url(url_string);

  WebDropData drop_data;
  drop_data.url = url;
  drop_data.is_bb_drag = true;

  // Empty title should be replaced with origin of url.
  BbDragData drag_data(drop_data);
  EXPECT_EQ(drag_data.url(), url);
  EXPECT_EQ(drag_data.title(), UTF8ToWide(drag_data.url().GetOrigin().spec()));
}

// Invalid URL should prevent initialization.
TEST_F(BbDragDataTest, Constructor4) {
  std::string url_string("app/bogus/123/foo?bar=yes");
  GURL url(url_string);
  std::wstring title(L"bad url");

  WebDropData drop_data;
  drop_data.url = url;
  drop_data.url_title = title;
  drop_data.is_bb_drag = true;

  BbDragData drag_data(drop_data);
  EXPECT_TRUE(drag_data.url().is_empty());
  EXPECT_TRUE(drag_data.title().empty());
}

// Read from empty OSExchangeData fails.
TEST_F(BbDragDataTest, BogusRead) {
  scoped_refptr<OSExchangeData> data(new OSExchangeData());

  std::string url_string("http://123.com/foo?bar=yes");
  GURL url(url_string);
  std::wstring title(L"trick or treat!");

  WebDropData drop_data;
  drop_data.url = url;
  drop_data.url_title = title;
  drop_data.is_bb_drag = true;

  BbDragData drag_data(drop_data);

  EXPECT_FALSE(drag_data.Read(data.get()));
  EXPECT_EQ(drag_data.url(), url);
  EXPECT_EQ(drag_data.title(), title);
}

// Roundtrip the data via OSExchangeData.
TEST_F(BbDragDataTest, Roundtrip) {
  std::string url_string("https://www.google.com/foo?bar=yes");
  std::wstring title(L"test_task with space");
  GURL url(url_string);

  WebDropData drop_data;
  drop_data.url = url;
  drop_data.url_title = title;
  drop_data.is_bb_drag = true;

  BbDragData drag_data(drop_data);
  EXPECT_EQ(drag_data.url(), url);
  EXPECT_EQ(drag_data.title(), title);

  scoped_refptr<OSExchangeData> data(new OSExchangeData());

  drag_data.Write(data.get());

  BbDragData drag_data_copy;
  EXPECT_TRUE(drag_data_copy.Read(data.get()));
  EXPECT_EQ(drag_data_copy.url(), drag_data.url());
  EXPECT_EQ(drag_data_copy.title(), drag_data.title());
}

#endif  // ENABLE_BACKGROUND_TASK



