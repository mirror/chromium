// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/supervised_user_error_page/supervised_user_error_page.h"

#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "components/grit/components_resources.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/webui/jstemplate_builder.h"
#include "ui/base/webui/web_ui_util.h"

namespace supervised_user_error_page {

namespace {

static const int kAvatarSize1x = 45;
static const int kAvatarSize2x = 90;

bool ReasonIsAutomatic(FilteringBehaviorReason reason) {
  return reason == ASYNC_CHECKER || reason == BLACKLIST;
}

std::string BuildAvatarImageUrl(const std::string& url, int size) {
  std::string result = url;
  size_t slash = result.rfind('/');
  if (slash != std::string::npos)
    result.insert(slash, "/s" + base::IntToString(size));
  return result;
}

}  //  namespace

int GetBlockMessageID(FilteringBehaviorReason reason,
                      bool is_child_account,
                      bool single_parent) {
  switch (reason) {
    case DEFAULT:
      if (!is_child_account)
        return IDS_SUPERVISED_USER_BLOCK_MESSAGE_DEFAULT;
      if (single_parent)
        return IDS_CHILD_BLOCK_MESSAGE_DEFAULT_SINGLE_PARENT;
      return IDS_CHILD_BLOCK_MESSAGE_DEFAULT_MULTI_PARENT;
    case BLACKLIST:
    case ASYNC_CHECKER:
      return IDS_SUPERVISED_USER_BLOCK_MESSAGE_SAFE_SITES;
    case WHITELIST:
      NOTREACHED();
      break;
    case MANUAL:
      if (!is_child_account)
        return IDS_SUPERVISED_USER_BLOCK_MESSAGE_MANUAL;
      if (single_parent)
        return IDS_CHILD_BLOCK_MESSAGE_MANUAL_SINGLE_PARENT;
      return IDS_CHILD_BLOCK_MESSAGE_MANUAL_MULTI_PARENT;
    case NOT_SIGNED_IN:
      return IDS_SUPERVISED_USER_NOT_SIGNED_IN;
  }
  NOTREACHED();
  return 0;
}

std::string BuildHtml(bool allow_access_requests,
                      const std::string& profile_image_url,
                      const std::string& profile_image_url2,
                      const std::string& custodian,
                      const std::string& custodian_email,
                      const std::string& second_custodian,
                      const std::string& second_custodian_email,
                      bool is_child_account,
                      bool is_deprecated,
                      FilteringBehaviorReason reason,
                      const std::string& app_locale) {
  base::DictionaryValue strings;
  strings.SetKey(
      "blockPageTitle",
      base::Value(l10n_util::GetStringUTF16(IDS_BLOCK_INTERSTITIAL_TITLE)));
  strings.SetKey("allowAccessRequests", base::Value(allow_access_requests));
  strings.SetKey("avatarURL1x", base::Value(BuildAvatarImageUrl(
                                    profile_image_url, kAvatarSize1x)));
  strings.SetKey("avatarURL2x", base::Value(BuildAvatarImageUrl(
                                    profile_image_url, kAvatarSize2x)));
  strings.SetKey("secondAvatarURL1x", base::Value(BuildAvatarImageUrl(
                                          profile_image_url2, kAvatarSize1x)));
  strings.SetKey("secondAvatarURL2x", base::Value(BuildAvatarImageUrl(
                                          profile_image_url2, kAvatarSize2x)));
  base::string16 custodian16 = base::UTF8ToUTF16(custodian);
  strings.SetKey("custodianName", base::Value(custodian16));
  strings.SetKey("custodianEmail",
                 base::Value(base::UTF8ToUTF16(custodian_email)));
  strings.SetKey("secondCustodianName",
                 base::Value(base::UTF8ToUTF16(second_custodian)));
  strings.SetKey("secondCustodianEmail",
                 base::Value(base::UTF8ToUTF16(second_custodian_email)));
  base::string16 block_header;
  base::string16 block_message;
  if (reason == FilteringBehaviorReason::NOT_SIGNED_IN) {
    block_header =
        l10n_util::GetStringUTF16(IDS_BLOCK_INTERSTITIAL_HEADER_NOT_SIGNED_IN);
  } else if (allow_access_requests) {
    if (is_child_account) {
      block_header =
          l10n_util::GetStringUTF16(IDS_CHILD_BLOCK_INTERSTITIAL_HEADER);
      block_message =
          l10n_util::GetStringUTF16(IDS_CHILD_BLOCK_INTERSTITIAL_MESSAGE);
    } else {
      block_header = l10n_util::GetStringFUTF16(IDS_BLOCK_INTERSTITIAL_HEADER,
                                                custodian16);
      // For non-child accounts, the block message is empty.
    }
  } else {
    block_header = l10n_util::GetStringUTF16(
        IDS_BLOCK_INTERSTITIAL_HEADER_ACCESS_REQUESTS_DISABLED);

    if (is_deprecated) {
      DCHECK(!is_child_account);
      block_message = l10n_util::GetStringUTF16(
          IDS_BLOCK_INTERSTITIAL_MESSAGE_SUPERVISED_USERS_DEPRECATED);
    }
  }
  strings.SetKey("blockPageHeader", base::Value(block_header));
  strings.SetKey("blockPageMessage", base::Value(block_message));
  strings.SetKey("blockReasonMessage",
                 base::Value(l10n_util::GetStringUTF16(GetBlockMessageID(
                     reason, is_child_account, second_custodian.empty()))));
  strings.SetKey(
      "blockReasonHeader",
      base::Value(l10n_util::GetStringUTF16(IDS_SUPERVISED_USER_BLOCK_HEADER)));
  bool show_feedback = ReasonIsAutomatic(reason);
  DCHECK(is_child_account || !show_feedback);

  strings.SetKey("showFeedbackLink", base::Value(show_feedback));
  strings.SetKey("feedbackLink", base::Value(l10n_util::GetStringUTF16(
                                     IDS_BLOCK_INTERSTITIAL_SEND_FEEDBACK)));
  strings.SetKey("backButton",
                 base::Value(l10n_util::GetStringUTF16(IDS_BACK_BUTTON)));
  strings.SetKey("requestAccessButton",
                 base::Value(l10n_util::GetStringUTF16(
                     IDS_BLOCK_INTERSTITIAL_REQUEST_ACCESS_BUTTON)));
  strings.SetKey("showDetailsLink", base::Value(l10n_util::GetStringUTF16(
                                        IDS_BLOCK_INTERSTITIAL_SHOW_DETAILS)));
  strings.SetKey("hideDetailsLink", base::Value(l10n_util::GetStringUTF16(
                                        IDS_BLOCK_INTERSTITIAL_HIDE_DETAILS)));
  base::string16 request_sent_message;
  base::string16 request_failed_message;
  if (is_child_account) {
    if (second_custodian.empty()) {
      request_sent_message = l10n_util::GetStringUTF16(
          IDS_CHILD_BLOCK_INTERSTITIAL_REQUEST_SENT_MESSAGE_SINGLE_PARENT);
      request_failed_message = l10n_util::GetStringUTF16(
          IDS_CHILD_BLOCK_INTERSTITIAL_REQUEST_FAILED_MESSAGE_SINGLE_PARENT);
    } else {
      request_sent_message = l10n_util::GetStringUTF16(
          IDS_CHILD_BLOCK_INTERSTITIAL_REQUEST_SENT_MESSAGE_MULTI_PARENT);
      request_failed_message = l10n_util::GetStringUTF16(
          IDS_CHILD_BLOCK_INTERSTITIAL_REQUEST_FAILED_MESSAGE_MULTI_PARENT);
    }
  } else {
    request_sent_message = l10n_util::GetStringFUTF16(
        IDS_BLOCK_INTERSTITIAL_REQUEST_SENT_MESSAGE, custodian16);
    request_failed_message = l10n_util::GetStringFUTF16(
        IDS_BLOCK_INTERSTITIAL_REQUEST_FAILED_MESSAGE, custodian16);
  }
  strings.SetKey("requestSentMessage", base::Value(request_sent_message));
  strings.SetKey("requestFailedMessage", base::Value(request_failed_message));
  webui::SetLoadTimeDataDefaults(app_locale, &strings);
  std::string html =
      ui::ResourceBundle::GetSharedInstance()
          .GetRawDataResource(IDR_SUPERVISED_USER_BLOCK_INTERSTITIAL_HTML)
          .as_string();
  webui::AppendWebUiCssTextDefaults(&html);
  std::string error_html = webui::GetI18nTemplateHtml(html, &strings);
  return error_html;
}

}  //  namespace supervised_user_error_page
