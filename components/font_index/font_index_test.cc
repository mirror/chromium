// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"

#include "components/font_index/font_index.h"


namespace font_index {

TEST(FontIndexTest, TestFontIndex) {
  FontIndexer font_indexer;
  font_indexer.ReadIndexFromFile();
  font_indexer.UpdateIndex();
  font_indexer.PersistIndexToFile();
}

}
