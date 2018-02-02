// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/font_index/font_index.h"

#include "base/files/file.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/logging.h"

#include <set>
#include <vector>

namespace font_index {

// https://www.microsoft.com/typography/otspec/name.htm#nameIDs
const unsigned kNameIdFontFamilyName = 1;  // e.g. "Arial"
// const unsigned kNameIdFontSubFamilyName = 2;  // e.g. "Bold"
// const unsigned kNameIdFullFontName = 4;       // e.g. "Arial Bold"
const unsigned kNameIdPostscriptName = 6;  // e.g. "ArialMT"

const char kFontPath[] = "components/font_index/android_fonts/";
const char kIndexPath[] = "font_indexer.pb";

FontIndexer::FontIndexer() : font_index_proto_() {
  FT_Init_FreeType(&ft_library_);
}

void FontIndexer::UpdateIndex() {
  // Scan the file system first.
  std::unique_ptr<std::vector<std::string>> known_font_files =
      CollectFontFiles();
  // Go through the cached list:
  // If the file does not exist, add it to a set of entries to be removed.
  // If a cached path does exist in the known_font_files, compare timestamp
  // and size.
  //   If they match, continue.
  //   If they do not match, reindex/add.
  //   Remove entry from known_font_files.
  // If files left in known_font_files, reindex/add.
  std::set<size_t> to_be_deleted;
  for (int i = 0; i < font_index_proto_.indexed_fonts_size(); ++i) {
    FontIndex_FontIndexEntry* cached_font_entry =
        font_index_proto_.mutable_indexed_fonts(i);

    auto found_file =
        std::find(known_font_files->begin(), known_font_files->end(),
                  cached_font_entry->font_file_path());
    if (found_file == known_font_files->end()) {
      to_be_deleted.insert(i);
    } else {
      known_font_files->erase(found_file);
      if (MatchesCache(cached_font_entry->font_file_path(),
                       cached_font_entry->font_file_size(),
                       cached_font_entry->font_file_timestamp())) {
        VLOG(4) << "Skipping " << cached_font_entry->font_file_path();
        continue;
      } else {
        IndexFile(cached_font_entry, cached_font_entry->font_file_path());
      }
    }
  }

  for (size_t element_to_be_deleted : to_be_deleted) {
    font_index_proto_.mutable_indexed_fonts()->DeleteSubrange(
        element_to_be_deleted, 1);
  }

  for (auto remaining_unscanned_file : *known_font_files) {
    IndexFile(font_index_proto_.add_indexed_fonts(), remaining_unscanned_file);
  }
}

void FontIndexer::PersistIndexToFile() {
  std::string proto_output;
  font_index_proto_.SerializeToString(&proto_output);
  base::File index_file(
      base::FilePath(kIndexPath),
      base::File::FLAG_CREATE_ALWAYS | base::File::Flags::FLAG_WRITE);
  CHECK(index_file.IsValid());
  index_file.Write(0, proto_output.c_str(), proto_output.length());
  index_file.Close();
}

void FontIndexer::ReadIndexFromFile() {
    base::File index_file(
      base::FilePath(kIndexPath),
      base::File::FLAG_OPEN | base::File::Flags::FLAG_READ);
    if (!index_file.IsValid())
      return;
    std::vector<char> file_contents;
    file_contents.resize(index_file.GetLength());
    index_file.Read(0, file_contents.data(), file_contents.size());
    std::string file_contents_string(file_contents.data(), file_contents.size());
    CHECK(font_index_proto_.ParseFromString(file_contents_string));
    index_file.Close();
}

bool FontIndexer::MatchesCache(const std::string& path,
                               uint64_t font_file_size,
                               uint64_t font_file_timestamp) {
    base::File font_file(
      base::FilePath(path),
      base::File::FLAG_OPEN | base::File::Flags::FLAG_READ);
    base::File::Info font_file_info;
    if (!font_file.IsValid())
      return false;
    font_file.GetInfo(&font_file_info);
    return static_cast<uint64_t>(font_file_info.size) == font_file_size &&
           static_cast<uint64_t>(
               font_file_info.last_modified.ToDeltaSinceWindowsEpoch()
                   .InMilliseconds()) == font_file_timestamp;
}

void FontIndexer::IndexFile(FontIndex_FontIndexEntry* font_index_entry,
                            const std::string& font_file_path) {
  VLOG(4) << "INDEXING " << font_file_path;
  FT_Face font_face;
  // TODO: Cleanup after FT_New_Face;
  CHECK_EQ(FT_New_Face(ft_library_, font_file_path.c_str(), 0, &font_face), 0);
  // Get file attributes
  // TODO: Error handling for open and attributes retrieval.
  base::File font_file_for_info(
      base::FilePath(font_file_path.c_str()),
      base::File::FLAG_OPEN | base::File::Flags::FLAG_READ);
  CHECK(font_file_for_info.IsValid());
  base::File::Info font_file_info;
  font_file_for_info.GetInfo(&font_file_info);
  for (size_t i = 0; i < FT_Get_Sfnt_Name_Count(font_face); ++i) {
    font_index_entry->set_font_file_path(font_file_path);
    font_index_entry->set_font_file_size(font_file_info.size);
    font_index_entry->set_font_file_timestamp(
        font_file_info.last_modified.ToDeltaSinceWindowsEpoch()
            .InMilliseconds());
    FT_SfntName sfnt_name;
    CHECK_EQ(FT_Get_Sfnt_Name(font_face, i, &sfnt_name), 0);
    std::string sfnt_name_string((char*)sfnt_name.string, sfnt_name.string_len);
    switch (sfnt_name.name_id) {
      case kNameIdFontFamilyName:
        font_index_entry->set_font_family_name(sfnt_name_string);
        break;
      case kNameIdPostscriptName:
        font_index_entry->set_font_postscript_name(sfnt_name_string);
        break;
      default:
        break;
    }
  }
}

std::unique_ptr<std::vector<std::string>> FontIndexer::CollectFontFiles() {
  auto font_files = std::make_unique<std::vector<std::string>>();
  base::FileEnumerator files_enumerator(base::FilePath(kFontPath), true,
                                        base::FileEnumerator::FILES);
  for (base::FilePath name = files_enumerator.Next(); !name.empty();
       name = files_enumerator.Next()) {
    if (name.Extension() == ".ttf" || name.Extension() == ".otf") {
      font_files->push_back(name.value());
    }
  }
  return font_files;
}

}
