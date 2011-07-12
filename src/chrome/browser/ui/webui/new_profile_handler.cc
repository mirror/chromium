// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/new_profile_handler.h"

#include "base/string_number_conversions.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_info_cache.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/url_constants.h"
#include "content/browser/tab_contents/tab_contents.h"

NewProfileHandler::NewProfileHandler() {
}

void NewProfileHandler::RegisterMessages() {
  web_ui_->RegisterMessageCallback("create",
      NewCallback(this, &NewProfileHandler::HandleCreate));
  web_ui_->RegisterMessageCallback("cancel",
      NewCallback(this, &NewProfileHandler::HandleCancel));
  web_ui_->RegisterMessageCallback("requestProfileInfo",
      NewCallback(this, &NewProfileHandler::HandleRequestProfileInfo));
}

void NewProfileHandler::HandleCreate(const ListValue* args) {
  string16 profile_name;
  bool ret = args->GetString(0, &profile_name);
  if (!ret || profile_name.empty())
    return;

  std::string string_index;
  ret = args->GetString(1, &string_index);
  if (!ret || string_index.empty())
    return;
  int profile_avatar_index;
  if (!base::StringToInt(string_index, &profile_avatar_index))
    return;

  ProfileInfoCache& cache =
      g_browser_process->profile_manager()->GetProfileInfoCache();
  size_t index = cache.GetIndexOfProfileWithPath(
      web_ui_->GetProfile()->GetPath());
  if (index != std::string::npos) {
    cache.SetNameOfProfileAtIndex(index, profile_name);
    cache.SetAvatarIconOfProfileAtIndex(index, profile_avatar_index);
  }

  web_ui_->tab_contents()->OpenURL(GURL(chrome::kChromeUINewTabURL),
                                   GURL(), CURRENT_TAB,
                                   PageTransition::LINK);
}

void NewProfileHandler::HandleCancel(const ListValue* args) {
  g_browser_process->profile_manager()->ScheduleProfileForDeletion(
      web_ui_->GetProfile()->GetPath());
}

void NewProfileHandler::HandleRequestProfileInfo(const ListValue* args) {
  SendDefaultAvatarImages();
  SendProfileInfo();
}

void NewProfileHandler::SendDefaultAvatarImages() {
  ListValue image_url_list;
  for (size_t i = 0; i < ProfileInfoCache::GetDefaultAvatarIconCount(); i++) {
    std::string url = ProfileInfoCache::GetDefaultAvatarIconUrl(i);
    image_url_list.Append(Value::CreateStringValue(url));
  }
  web_ui_->CallJavascriptFunction("newProfile.setDefaultAvatarImages",
                                  image_url_list);
}

void NewProfileHandler::SendProfileInfo() {
  ProfileInfoCache& cache =
      g_browser_process->profile_manager()->GetProfileInfoCache();
  size_t index = cache.GetIndexOfProfileWithPath(
      web_ui_->GetProfile()->GetPath());
  if (index != std::string::npos) {
    StringValue profile_name(cache.GetNameOfProfileAtIndex(index));
    FundamentalValue profile_icon_index(static_cast<int>(
        cache.GetAvatarIconIndexOfProfileAtIndex(index)));
    web_ui_->CallJavascriptFunction("newProfile.setProfileInfo",
                                    profile_name,
                                    profile_icon_index);
  }
}
