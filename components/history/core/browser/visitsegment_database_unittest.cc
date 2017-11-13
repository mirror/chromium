// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/history/core/browser/visitsegment_database.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace history {
namespace {

using ::testing::ElementsAre;

TEST(VisitSegmentDatabaseTest, ComputeSegmentNames) {
  EXPECT_THAT(
      VisitSegmentDatabase::ComputeSegmentNames(GURL("http://google.com/")),
      ElementsAre("http://google.com/"));

  // www. prefix should be stripped.
  EXPECT_THAT(
      VisitSegmentDatabase::ComputeSegmentNames(GURL("http://www.google.com/")),
      ElementsAre("http://google.com/"));

  // No prefix stripping for weird hostnames.
  EXPECT_THAT(VisitSegmentDatabase::ComputeSegmentNames(GURL("http://www/")),
              ElementsAre("http://www/"));
  EXPECT_THAT(VisitSegmentDatabase::ComputeSegmentNames(GURL("http://www./")),
              ElementsAre("http://www./"));

  // Mobile prefixes should be stripped.
  EXPECT_THAT(
      VisitSegmentDatabase::ComputeSegmentNames(GURL("http://m.google.com/")),
      ElementsAre("http://google.com/", "http://m.google.com/"));
  EXPECT_THAT(VisitSegmentDatabase::ComputeSegmentNames(
                  GURL("http://mobile.google.com/")),
              ElementsAre("http://google.com/", "http://mobile.google.com/"));

  // https should be canonicalized to http.
  EXPECT_THAT(
      VisitSegmentDatabase::ComputeSegmentNames(GURL("https://google.com/")),
      ElementsAre("http://google.com/", "https://google.com/"));

  // ftp should NOT be canonicalized to http.
  EXPECT_THAT(
      VisitSegmentDatabase::ComputeSegmentNames(GURL("ftp://google.com/")),
      ElementsAre("ftp://google.com/"));

  // Combined scheme canonicalization AND prefix stripping.
  EXPECT_THAT(VisitSegmentDatabase::ComputeSegmentNames(
                  GURL("https://www.google.com/")),
              ElementsAre("http://google.com/", "https://google.com/"));
  EXPECT_THAT(
      VisitSegmentDatabase::ComputeSegmentNames(GURL("https://m.google.com/")),
      ElementsAre("http://google.com/", "https://m.google.com/"));

  // Should not strip multiple prefixes.
  EXPECT_THAT(
      VisitSegmentDatabase::ComputeSegmentNames(GURL("http://www.m.foo/")),
      ElementsAre("http://m.foo/"));
  EXPECT_THAT(
      VisitSegmentDatabase::ComputeSegmentNames(GURL("http://m.www.foo/")),
      ElementsAre("http://www.foo/", "http://m.www.foo/"));
  EXPECT_THAT(
      VisitSegmentDatabase::ComputeSegmentNames(GURL("http://m.mobile.foo/")),
      ElementsAre("http://mobile.foo/", "http://m.mobile.foo/"));
}

}  // namespace
}  // namespace history
