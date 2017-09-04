// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/disposition_utils.h"

#include <memory>

#include "base/supports_user_data.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/window_open_disposition.h"

namespace chrome {

namespace {

const char kDispositionUserDataKey[] = "DispositionUserData";

class DispositionUserData : public base::SupportsUserData::Data {
 public:
  explicit DispositionUserData(WindowOpenDisposition disposition)
      : disposition_(disposition) {}
  ~DispositionUserData() override {}
  WindowOpenDisposition disposition() { return disposition_; }

 private:
  WindowOpenDisposition disposition_;
};

class IsFromAppUserData : public base::SupportsUserData::Data {
 public:
  explicit IsFromAppUserData(bool is_from_app) : is_from_app_(is_from_app) {}
  bool is_from_app() { return is_from_app_; }

 private:
  bool is_from_app_;
};

}  // namespace

WindowOpenDisposition GetDisposition(content::WebContents* tab) {
  if (!tab)
    return WindowOpenDisposition::UNKNOWN;

  DispositionUserData* user_data = static_cast<DispositionUserData*>(
      tab->GetUserData(&kDispositionUserDataKey));

  return user_data ? user_data->disposition() : WindowOpenDisposition::UNKNOWN;
}

void SetDisposition(content::WebContents* tab,
                    WindowOpenDisposition disposition) {
  tab->SetUserData(&kDispositionUserDataKey,
                   std::make_unique<DispositionUserData>(disposition));
}

}  // namespace chrome
