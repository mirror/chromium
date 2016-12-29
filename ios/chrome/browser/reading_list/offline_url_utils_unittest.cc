// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/reading_list/offline_url_utils.h"

#include <string>

#include "base/files/file_path.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/gtest_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// Checks the distilled URL for the page is chrome://offline/MD5/page.html;
TEST(OfflineURLUtilsTest, DistilledURLForPathTest) {
  base::FilePath page_path("MD5/page.html");
  GURL distilled_url = reading_list::DistilledURLForPath(page_path, GURL());
  EXPECT_EQ("chrome://offline/MD5/page.html", distilled_url.spec());
}

// Checks the distilled URL for the page with an onlineURL is
// chrome://offline/MD5/page.html?virtualURL=encorded%20URL;
TEST(OfflineURLUtilsTest, DistilledURLForPathWithVirtualURLTest) {
  base::FilePath page_path("MD5/page.html");
  GURL online_url = GURL("http://foo.bar");
  GURL distilled_url = reading_list::DistilledURLForPath(page_path, online_url);
  EXPECT_EQ("chrome://offline/MD5/page.html?virtualURL=http%3A%2F%2Ffoo.bar%2F",
            distilled_url.spec());
}

// Checks the distilled URL for the page is chrome://offline/MD5/page.html;
TEST(OfflineURLUtilsTest, VirtualURLForDistilledURLTest) {
  GURL distilled_url("chrome://offline/MD5/page.html");
  GURL virtual_url = reading_list::VirtualURLForDistilledURL(distilled_url);
  EXPECT_EQ("chrome://offline/MD5/page.html", virtual_url.spec());
}

// Checks the distilled URL for the page with an onlineURL is
// chrome://offline/MD5/page.html?virtualURL=encorded%20URL;
TEST(OfflineURLUtilsTest, VirtualURLForDistilledURLWithVirtualURLTest) {
  GURL distilled_url(
      "chrome://offline/MD5/page.html?virtualURL=http%3A%2F%2Ffoo.bar%2F");
  GURL virtual_url = reading_list::VirtualURLForDistilledURL(distilled_url);
  EXPECT_EQ("http://foo.bar/", virtual_url.spec());
}

// Checks the file path for chrome://offline/MD5/page.html is
// file://profile_path/Offline/MD5/page.html.
// Checks the resource root for chrome://offline/MD5/page.html is
// file://profile_path/Offline/MD5
TEST(OfflineURLUtilsTest, FileURLForDistilledURLTest) {
  base::FilePath offline_path("/profile_path/Offline");
  GURL file_url =
      reading_list::FileURLForDistilledURL(GURL(), offline_path, nullptr);
  EXPECT_FALSE(file_url.is_valid());

  GURL distilled_url("chrome://offline/MD5/page.html");
  file_url = reading_list::FileURLForDistilledURL(distilled_url, offline_path,
                                                  nullptr);
  EXPECT_TRUE(file_url.is_valid());
  EXPECT_TRUE(file_url.SchemeIsFile());
  EXPECT_EQ("/profile_path/Offline/MD5/page.html", file_url.path());

  GURL resource_url;
  file_url = reading_list::FileURLForDistilledURL(distilled_url, offline_path,
                                                  &resource_url);
  EXPECT_TRUE(resource_url.is_valid());
  EXPECT_TRUE(resource_url.SchemeIsFile());
  EXPECT_EQ("/profile_path/Offline/MD5/", resource_url.path());
}

// Checks that the offline URLs are correctly detected by |IsOfflineURL|.
TEST(OfflineURLUtilsTest, IsOfflineURL) {
  EXPECT_FALSE(reading_list::IsOfflineURL(GURL()));
  EXPECT_FALSE(reading_list::IsOfflineURL(GURL("chrome://")));
  EXPECT_FALSE(reading_list::IsOfflineURL(GURL("chrome://offline-foobar")));
  EXPECT_FALSE(reading_list::IsOfflineURL(GURL("http://offline/")));
  EXPECT_FALSE(reading_list::IsOfflineURL(GURL("http://chrome://offline/")));
  EXPECT_TRUE(reading_list::IsOfflineURL(GURL("chrome://offline")));
  EXPECT_TRUE(reading_list::IsOfflineURL(GURL("chrome://offline/")));
  EXPECT_TRUE(reading_list::IsOfflineURL(GURL("chrome://offline/foobar")));
  EXPECT_TRUE(
      reading_list::IsOfflineURL(GURL("chrome://offline/foobar?foo=bar")));
}

// Checks that https:// scheme is correctly removed by
// StripSchemeFromOnlineURLTest.
TEST(OfflineURLUtilsTest, StripSchemeFromOnlineURLTest) {
  size_t removed_size;
  base::string16 empty_url;
  EXPECT_EQ(reading_list::StripSchemeFromOnlineURL(empty_url, &removed_size),
            empty_url);
  EXPECT_EQ(removed_size, 0u);

  base::string16 https_url = base::UTF8ToUTF16("https://www.chromium.org/");
  base::string16 trimmed_https_url = base::UTF8ToUTF16("www.chromium.org/");
  EXPECT_EQ(reading_list::StripSchemeFromOnlineURL(https_url, &removed_size),
            trimmed_https_url);
  EXPECT_EQ(removed_size, 8u);
  base::string16 http_url = base::UTF8ToUTF16("http://www.chromium.org/");
  EXPECT_DCHECK_DEATH(
      reading_list::StripSchemeFromOnlineURL(http_url, &removed_size));

  base::string16 other_scheme_url =
      base::UTF8ToUTF16("scheme://www.chromium.org/");
  EXPECT_EQ(
      reading_list::StripSchemeFromOnlineURL(other_scheme_url, &removed_size),
      other_scheme_url);
  EXPECT_EQ(removed_size, 0u);

  base::string16 no_scheme_url = base::UTF8ToUTF16("www.chromium.org/");
  EXPECT_EQ(
      reading_list::StripSchemeFromOnlineURL(no_scheme_url, &removed_size),
      no_scheme_url);
  EXPECT_EQ(removed_size, 0u);
}
