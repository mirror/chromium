// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/hosted_app_frame_header_ash.h"

#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(HostedAppFrameHeaderAshTest, GetRenderTexts) {
  base::string16 title_label_text;
  base::string16 app_and_domain_label_text;

  // Normal case with distinct title and app name.
  HostedAppFrameHeaderAsh::GetRenderTexts(
      base::UTF8ToUTF16("My Document"), base::UTF8ToUTF16("My App"),
      base::UTF8ToUTF16("My App by example.com"), &title_label_text,
      &app_and_domain_label_text);
  EXPECT_EQ(base::UTF8ToUTF16("My Document"), title_label_text);
  EXPECT_EQ(base::UTF8ToUTF16(" | My App by example.com"),
            app_and_domain_label_text);

  // Title equals app name.
  HostedAppFrameHeaderAsh::GetRenderTexts(
      base::UTF8ToUTF16("My App"), base::UTF8ToUTF16("My App"),
      base::UTF8ToUTF16("My App by example.com"), &title_label_text,
      &app_and_domain_label_text);
  EXPECT_EQ(base::string16(), title_label_text);
  EXPECT_EQ(base::UTF8ToUTF16("My App by example.com"),
            app_and_domain_label_text);

  // Title ends with app name.
  HostedAppFrameHeaderAsh::GetRenderTexts(
      base::UTF8ToUTF16("My Document - My App"), base::UTF8ToUTF16("My App"),
      base::UTF8ToUTF16("My App by example.com"), &title_label_text,
      &app_and_domain_label_text);
  EXPECT_EQ(base::UTF8ToUTF16("My Document"), title_label_text);
  EXPECT_EQ(base::UTF8ToUTF16(" | My App by example.com"),
            app_and_domain_label_text);

  // Title ends with start of app name (not special-cased).
  HostedAppFrameHeaderAsh::GetRenderTexts(
      base::UTF8ToUTF16("My Document - My App"),
      base::UTF8ToUTF16("My App, My Life"),
      base::UTF8ToUTF16("My App, My Life by example.com"), &title_label_text,
      &app_and_domain_label_text);
  EXPECT_EQ(base::UTF8ToUTF16("My Document - My App"), title_label_text);
  EXPECT_EQ(base::UTF8ToUTF16(" | My App, My Life by example.com"),
            app_and_domain_label_text);

  // Title, app name and origin are equal (not special-cased).
  HostedAppFrameHeaderAsh::GetRenderTexts(
      base::UTF8ToUTF16("example.com"), base::UTF8ToUTF16("example.com"),
      base::UTF8ToUTF16("example.com by example.com"), &title_label_text,
      &app_and_domain_label_text);
  EXPECT_EQ(base::string16(), title_label_text);
  EXPECT_EQ(base::UTF8ToUTF16("example.com by example.com"),
            app_and_domain_label_text);

  // Different separators and punctuation (all should be deleted).
  HostedAppFrameHeaderAsh::GetRenderTexts(
      base::UTF8ToUTF16("My Document, -| ⨠ ↓ \t My App"),
      base::UTF8ToUTF16("My App"), base::UTF8ToUTF16("My App by example.com"),
      &title_label_text, &app_and_domain_label_text);
  EXPECT_EQ(base::UTF8ToUTF16("My Document"), title_label_text);
  EXPECT_EQ(base::UTF8ToUTF16(" | My App by example.com"),
            app_and_domain_label_text);
}
