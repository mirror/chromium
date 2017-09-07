// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/extra_navigation_info_utils.h"

#include <memory>
#include <tuple>

#include "base/supports_user_data.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/window_open_disposition.h"

namespace chrome {

namespace {

const char kExtraNavigationInfoUserDataKey[] = "NavigationInfoUserData";

class ExtraNavigationInfoUserData : public base::SupportsUserData::Data {
 public:
  explicit ExtraNavigationInfoUserData(WindowOpenDisposition disposition,
                                       bool was_target_contents_null,
                                       bool is_from_app)
      : disposition_(disposition),
        was_target_contents_null_(was_target_contents_null),
        is_from_app_(is_from_app) {}
  ~ExtraNavigationInfoUserData() override {}
  WindowOpenDisposition disposition() { return disposition_; }
  bool was_target_contents_null() { return was_target_contents_null_; }
  bool is_from_app() { return is_from_app_; }

 private:
  WindowOpenDisposition disposition_;
  bool was_target_contents_null_;
  bool is_from_app_;
};

}  // namespace

// TODO(before landing): Return more structured data.
std::tuple<WindowOpenDisposition, bool, bool> GetExtraNavigationInfo(
    content::WebContents* tab) {
  ExtraNavigationInfoUserData* user_data =
      static_cast<ExtraNavigationInfoUserData*>(
          tab->GetUserData(&kExtraNavigationInfoUserDataKey));

  return user_data
             ? std::make_tuple(user_data->disposition(),
                               user_data->was_target_contents_null(),
                               user_data->is_from_app())
             : std::make_tuple(WindowOpenDisposition::UNKNOWN, false, false);
}

void SetExtraNavigationInfo(content::WebContents* tab,
                            WindowOpenDisposition disposition,
                            bool was_target_contents_null,
                            bool is_from_app) {
  tab->SetUserData(&kExtraNavigationInfoUserDataKey,
                   std::make_unique<ExtraNavigationInfoUserData>(
                       disposition, was_target_contents_null, is_from_app));
}

}  // namespace chrome
