// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/page_renovation_loader.h"

#include <utility>

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"

namespace offline_pages {

namespace {

// Construct list of implemented renovations
std::vector<std::unique_ptr<PageRenovation>> makeRenovationList() {
  // TODO(collinbaker): Create PageRenovation instances and put them
  // in this list.
  return std::vector<std::unique_ptr<PageRenovation>>();
}

// "Null" script for if loading from storage fails.
const char kEmptyScript[] = "function run_renovations(flist){}";

}  // namespace

PageRenovationLoader::PageRenovationLoader()
    : renovations_(makeRenovationList()),
      is_loaded_(false),
      combined_source_(base::UTF8ToUTF16(kEmptyScript)) {}

PageRenovationLoader::~PageRenovationLoader() {}

base::string16 PageRenovationLoader::GetRenovationScript(
    const std::vector<std::string>& renovations) {
  LoadSource();

  // Create call to run_renovations function in JavaScript file.
  // List of renovation names are passed as a JavaScript array.
  std::string jsCall = "\nrun_renovations([";
  for (const std::string& funcName : renovations) {
    jsCall += "\"";
    jsCall += funcName;
    jsCall += "\",";
  }
  jsCall += "]);";

  // Append run_renovations call to combined_source_, which contains
  // the definition of run_renovations.
  base::string16 script = combined_source_;
  script += base::UTF8ToUTF16(jsCall);
  return script;
}

void PageRenovationLoader::LoadSource() {
  // We only need to load the script from storage once.
  if (is_loaded_) {
    return;
  }

  // TODO(collinbaker): Load file with renovations using
  // ui::ResourceBundle. For now, using temporary script that does
  // nothing.
  combined_source_ = base::UTF8ToUTF16(kEmptyScript);

  is_loaded_ = true;
}

void PageRenovationLoader::SetSourceForTest(base::string16 combined_source) {
  combined_source_ = std::move(combined_source);
  is_loaded_ = true;
}

void PageRenovationLoader::SetRenovationsForTest(
    std::vector<std::unique_ptr<PageRenovation>> renovations) {
  renovations_ = std::move(renovations);
}

}  // namespace offline_pages
