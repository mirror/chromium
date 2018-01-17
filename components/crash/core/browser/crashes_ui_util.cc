// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/crash/core/browser/crashes_ui_util.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/i18n/time_formatting.h"
#include "base/macros.h"
#include "base/values.h"
#include "components/strings/grit/components_chromium_strings.h"
#include "components/strings/grit/components_strings.h"
#include "components/upload_list/upload_list.h"

namespace crash {

const CrashesUILocalizedString kCrashesUILocalizedStrings[] = {
    {"bugLinkText", IDS_CRASH_BUG_LINK_LABEL},
    {"crashCountFormat", IDS_CRASH_CRASH_COUNT_BANNER_FORMAT},
    {"crashHeaderFormat", IDS_CRASH_CRASH_HEADER_FORMAT},
    {"crashHeaderFormatLocalOnly", IDS_CRASH_CRASH_HEADER_FORMAT_LOCAL_ONLY},
    {"crashUploadTimeFormat", IDS_CRASH_UPLOAD_TIME_FORMAT},
    {"crashCaptureAndUploadTimeFormat",
     IDS_CRASH_CAPTURE_AND_UPLOAD_TIME_FORMAT},
    {"crashNotUploaded", IDS_CRASH_CRASH_NOT_UPLOADED},
    {"crashUserRequested", IDS_CRASH_CRASH_USER_REQUESTED},
    {"crashPending", IDS_CRASH_CRASH_PENDING},
    {"crashesTitle", IDS_CRASH_TITLE},
    {"disabledHeader", IDS_CRASH_DISABLED_HEADER},
    {"disabledMessage", IDS_CRASH_DISABLED_MESSAGE},
    {"noCrashesMessage", IDS_CRASH_NO_CRASHES_MESSAGE},
    {"uploadCrashesLinkText", IDS_CRASH_UPLOAD_MESSAGE},
    {"uploadNowLinkText", IDS_CRASH_UPLOAD_NOW_LINK_TEXT},
    {"crashSizeMessage", IDS_CRASH_SIZE_MESSAGE},
};

const size_t kCrashesUILocalizedStringsCount =
    arraysize(kCrashesUILocalizedStrings);

const char kCrashesUICrashesJS[] = "crashes.js";
const char kCrashesUIRequestCrashList[] = "requestCrashList";
const char kCrashesUIRequestCrashUpload[] = "requestCrashUpload";
const char kCrashesUIShortProductName[] = "shortProductName";
const char kCrashesUIUpdateCrashList[] = "updateCrashList";
const char kCrashesUIRequestSingleCrashUpload[] = "requestSingleCrashUpload";

std::string UploadInfoStateAsString(UploadList::UploadInfo::State state) {
  switch (state) {
    case UploadList::UploadInfo::State::NotUploaded:
      return "not_uploaded";
    case UploadList::UploadInfo::State::Pending:
      return "pending";
    case UploadList::UploadInfo::State::Pending_UserRequested:
      return "pending_user_requested";
    case UploadList::UploadInfo::State::Uploaded:
      return "uploaded";
  }

  NOTREACHED();
  return "";
}

void UploadListToValue(UploadList* upload_list, base::ListValue* out_value) {
  std::vector<UploadList::UploadInfo> crashes;
  upload_list->GetUploads(50, &crashes);

  for (const auto& info : crashes) {
    std::unique_ptr<base::DictionaryValue> crash(new base::DictionaryValue());
    crash->SetKey("id", base::Value(info.upload_id));
    if (info.state == UploadList::UploadInfo::State::Uploaded) {
      crash->SetKey(
          "upload_time",
          base::Value(base::TimeFormatFriendlyDateAndTime(info.upload_time)));
    }
    if (!info.capture_time.is_null()) {
      crash->SetKey(
          "capture_time",
          base::Value(base::TimeFormatFriendlyDateAndTime(info.capture_time)));
    }
    crash->SetKey("local_id", base::Value(info.local_id));
    crash->SetKey("state", base::Value(UploadInfoStateAsString(info.state)));
    crash->SetKey("file_size", base::Value(info.file_size));
    out_value->Append(std::move(crash));
  }
}

}  // namespace crash
