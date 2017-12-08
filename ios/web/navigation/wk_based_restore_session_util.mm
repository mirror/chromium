// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/navigation/wk_based_restore_session_util.h"

#include "base/json/json_writer.h"
#include "base/mac/bundle_locations.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "base/values.h"
#import "ios/web/public/navigation_item.h"
#include "net/base/url_util.h"

static const char kSessionQueryKey[] = "session";
static const char kTargetUrlQueryKey[] = "targetUrl";

namespace web {

GURL GetRestoreSessionBaseUrl() {
  std::string restore_session_resource_path = base::SysNSStringToUTF8(
      [base::mac::FrameworkBundle() pathForResource:@"restore_session"
                                             ofType:@"html"]);
  return GURL(base::StringPrintf("%s://%s", url::kFileScheme,
                                 restore_session_resource_path.c_str()));
}

GURL CreateRestoreSessionUrl(
    int last_committed_item_index,
    const std::vector<std::unique_ptr<NavigationItem>>& items) {
  DCHECK(last_committed_item_index >= 0 &&
         last_committed_item_index < static_cast<int>(items.size()));

  // The URLs and titles of the restored entries are stored in two separate
  // lists instead of a single list of objects to reduce the size of the JSON
  // string to be included in the query parameter.
  base::Value restored_urls(base::Value::Type::LIST);
  base::Value restored_titles(base::Value::Type::LIST);
  restored_urls.GetList().reserve(items.size());
  restored_titles.GetList().reserve(items.size());
  for (size_t index = 0; index < items.size(); index++) {
    restored_urls.GetList().push_back(
        base::Value(items[index]->GetURL().spec()));
    restored_titles.GetList().push_back(base::Value(items[index]->GetTitle()));
  }
  base::Value session(base::Value::Type::DICTIONARY);
  int offset = last_committed_item_index + 1 - items.size();
  session.SetKey("offset", base::Value(offset));
  session.SetKey("urls", std::move(restored_urls));
  session.SetKey("titles", std::move(restored_titles));

  std::string session_json;
  base::JSONWriter::Write(session, &session_json);
  return net::AppendQueryParameter(GetRestoreSessionBaseUrl(), kSessionQueryKey,
                                   session_json);
}

bool IsRestoreSessionUrl(const GURL& url) {
  return url.SchemeIsFile() && url.path() == GetRestoreSessionBaseUrl().path();
}

bool ExtractTargetURL(const GURL& restore_session_url, GURL* target_url) {
  DCHECK(IsRestoreSessionUrl(restore_session_url));
  std::string target_url_spec;
  bool success = net::GetValueForKeyInQuery(
      restore_session_url, kTargetUrlQueryKey, &target_url_spec);
  if (success)
    *target_url = GURL(target_url_spec);

  return success;
}

}  // namespace web
