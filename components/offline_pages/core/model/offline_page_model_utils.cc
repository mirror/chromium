// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/offline_page_model_utils.h"

#include <string>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/i18n/file_util_icu.h"
#include "base/logging.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/task_scheduler/post_task.h"
#include "build/build_config.h"
#include "components/offline_pages/core/client_namespace_constants.h"
#include "components/offline_pages/core/offline_page_item.h"

#if defined(OS_WIN)
#include "base/strings/utf_string_conversions.h"
#endif

namespace offline_pages {

namespace {

const base::FilePath::CharType kMHTMLExtension[] = FILE_PATH_LITERAL("mhtml");
const base::FilePath::CharType kFileNameForEmptyTitle[] =
    FILE_PATH_LITERAL("NO TITLE.mhtml");

base::FilePath GenerateUniqueFileNameSync(const base::FilePath& target_dir,
                                          const GURL& url,
                                          const base::string16& title) {
  // TODO(romax): fallback to url once the function for generating file names
  // from urls are moved into base::.
  if (title.size() == 0)
    return base::FilePath(target_dir.Append(kFileNameForEmptyTitle));

  base::FilePath path_from_title = base::FilePath::FromUTF16Unsafe(title);
  base::FilePath::StringType file_name = path_from_title.value();
  base::i18n::ReplaceIllegalCharactersInPath(&file_name, '_');

  base::FilePath path_to_check(
      target_dir.Append(file_name).AddExtension(kMHTMLExtension));

  // Returns 0 if path_to_check is available; -1 if there's an error; other
  // positive number for uniquifier.
  int uniquifier =
      base::GetUniquePathNumber(path_to_check, base::FilePath::StringType());

  if (uniquifier < 0)
    return base::FilePath();

  if (uniquifier > 0) {
    path_to_check = path_to_check.InsertBeforeExtensionASCII(
        base::StringPrintf(" (%d)", uniquifier));
  }
  return path_to_check;
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
                            const base::string16& title,
                            GenerateUniqueFileNameCallback callback) {
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::BindOnce(&GenerateUniqueFileNameSync, target_dir, url, title),
      std::move(callback));
}

}  // namespace model_utils

}  // namespace offline_pages
