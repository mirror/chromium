// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/offline_page_model_utils.h"

#include <string>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/task_scheduler/post_task.h"
#include "components/offline_pages/core/client_namespace_constants.h"
#include "components/offline_pages/core/offline_page_item.h"

namespace offline_pages {

namespace {

const int kTitleMaxByteSize = 40;
const int kMaxUniqueFiles = 100;
const char kFileNameComponentsSeparator[] = "-";
const char kReplaceChars[] = " ";
const char kReplaceWith[] = "_";
const base::FilePath::CharType kMHTMLExtenstion[] = FILE_PATH_LITERAL("mhtml");

base::FilePath GenerateUniqueFileNameSync(const base::FilePath& target_dir,
                                          const GURL& url,
                                          const std::string& title) {
  base::FilePath::StringType title_part;
  base::TruncateUTF8ToByteSize(title, kTitleMaxByteSize, &title_part);
  std::string suggested_name(url.host() + kFileNameComponentsSeparator +
                             title_part);
  base::ReplaceChars(suggested_name, kReplaceChars, kReplaceWith,
                     &suggested_name);
  base::FilePath path_to_check(
      target_dir.Append(suggested_name).AddExtension(kMHTMLExtenstion));

  // If there's no duplicate names, the |path_to_check| is available.
  if (!base::PathExists(path_to_check))
    return path_to_check;
  // Try to add suffixes to |path_to_check| until an available name is found or
  // the limit is hit.
  for (int file_number = 1; file_number < kMaxUniqueFiles; ++file_number) {
    std::string suffix(base::StringPrintf(" (%d)", file_number));
    base::FilePath path_with_counter =
        path_to_check.InsertBeforeExtensionASCII(suffix);
    if (!base::PathExists(path_with_counter))
      return path_with_counter;
  }
  // If there's no available file name, just return an empty path.
  return base::FilePath();
}

}  // namespace

namespace model_utils {

OfflinePagesNamespaceEnumeration ToNamespaceEnum(
    const std::string& name_space) {
  if (name_space == kDefaultNamespace)
    return OfflinePagesNamespaceEnumeration::DEFAULT;
  else if (name_space == kBookmarkNamespace)
    return OfflinePagesNamespaceEnumeration::BOOKMARK;
  else if (name_space == kLastNNamespace)
    return OfflinePagesNamespaceEnumeration::LAST_N;
  else if (name_space == kAsyncNamespace)
    return OfflinePagesNamespaceEnumeration::ASYNC_LOADING;
  else if (name_space == kCCTNamespace)
    return OfflinePagesNamespaceEnumeration::CUSTOM_TABS;
  else if (name_space == kDownloadNamespace)
    return OfflinePagesNamespaceEnumeration::DOWNLOAD;
  else if (name_space == kNTPSuggestionsNamespace)
    return OfflinePagesNamespaceEnumeration::NTP_SUGGESTION;
  else if (name_space == kSuggestedArticlesNamespace)
    return OfflinePagesNamespaceEnumeration::SUGGESTED_ARTICLES;
  else if (name_space == kBrowserActionsNamespace)
    return OfflinePagesNamespaceEnumeration::BROWSER_ACTIONS;

  NOTREACHED();
  return OfflinePagesNamespaceEnumeration::DEFAULT;
}

std::string AddHistogramSuffix(const ClientId& client_id,
                               const char* histogram_name) {
  if (client_id.name_space.empty()) {
    NOTREACHED();
    return histogram_name;
  }
  std::string adjusted_histogram_name(histogram_name);
  adjusted_histogram_name += ".";
  adjusted_histogram_name += client_id.name_space;
  return adjusted_histogram_name;
}

void GenerateUniqueFileName(const base::FilePath& target_dir,
                            const GURL& url,
                            const std::string& title,
                            GenerateUniqueFileNameCallback callback) {
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::BindOnce(&GenerateUniqueFileNameSync, target_dir, url, title),
      std::move(callback));
}

}  // namespace model_utils

}  // namespace offline_pages
