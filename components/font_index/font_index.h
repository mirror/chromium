// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FONT_INDEX_H
#define COMPONENTS_FONT_INDEX_H


#include "components/font_index/font_index.pb.h"

#include <ft2build.h>
#include FT_SYSTEM_H
#include FT_TRUETYPE_TABLES_H
#include FT_SFNT_NAMES_H

#include <string>

namespace font_index {

class FontIndexer {
 public:
  FontIndexer();

  void UpdateIndex();

  void ReadIndexFromFile();
  void PersistIndexToFile();

 private:

  bool FileExists(const std::string& path);

  bool MatchesCache(const std::string& path,
                    uint64_t font_file_size,
                    uint64_t font_file_timestamp);

  void IndexFile(FontIndex_FontIndexEntry* font_index_entry,
                 const std::string& font_file_path);

  std::unique_ptr<std::vector<std::string>> CollectFontFiles();

  FontIndex font_index_proto_;
  FT_Library ft_library_;
};
}  // namespace font_index

#endif
