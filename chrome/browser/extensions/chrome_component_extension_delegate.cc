// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/chrome_component_extension_delegate.h"

#include "base/logging.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/grit/chrome_unscaled_resources.h"
#include "chrome/grit/component_extension_resources_map.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/browser_context.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/webui/web_ui_util.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/arc/arc_util.h"
#include "components/chrome_apps/chrome_apps_resource_util.h"
#include "ui/file_manager/file_manager_resource_util.h"
#include "ui/file_manager/grit/file_manager_resources.h"
#endif

#if defined(USE_AURA)
#include "ui/keyboard/content/keyboard_content_util.h"
#endif

namespace extensions {

ChromeComponentExtensionDelegate::ChromeComponentExtensionDelegate() {
  static const GritResourceMap kExtraComponentExtensionResources[] = {
    {"web_store/webstore_icon_128.png", IDR_WEBSTORE_ICON},
    {"web_store/webstore_icon_16.png", IDR_WEBSTORE_ICON_16},
    {"chrome_app/product_logo_128.png", IDR_PRODUCT_LOGO_128},
    {"chrome_app/product_logo_16.png", IDR_PRODUCT_LOGO_16},
#if defined(OS_CHROMEOS)
    {"webstore_widget/app/icons/icon_16.png", IDR_WEBSTORE_ICON_16},
    {"webstore_widget/app/icons/icon_32.png", IDR_WEBSTORE_ICON_32},
    {"webstore_widget/app/icons/icon_128.png", IDR_WEBSTORE_ICON},
#endif
  };

  AddComponentResourceEntries(
      kComponentExtensionResources,
      kComponentExtensionResourcesSize);
  AddComponentResourceEntries(
      kExtraComponentExtensionResources,
      arraysize(kExtraComponentExtensionResources));
#if defined(OS_CHROMEOS)
  size_t chrome_apps_resource_size;
  const GritResourceMap* chrome_apps_resources =
      chrome_apps::GetChromeAppsResources(&chrome_apps_resource_size);
  AddComponentResourceEntries(
      chrome_apps_resources,
      chrome_apps_resource_size);

  size_t file_manager_resource_size;
  const GritResourceMap* file_manager_resources =
      file_manager::GetFileManagerResources(&file_manager_resource_size);
  AddComponentResourceEntries(
      file_manager_resources,
      file_manager_resource_size);

  size_t keyboard_resource_size;
  const GritResourceMap* keyboard_resources =
      keyboard::GetKeyboardExtensionResources(&keyboard_resource_size);
  AddComponentResourceEntries(
      keyboard_resources,
      keyboard_resource_size);
#endif
}

ChromeComponentExtensionDelegate::~ChromeComponentExtensionDelegate() {}

bool ChromeComponentExtensionDelegate::IsComponentExtensionResource(
    const base::FilePath& extension_path,
    const base::FilePath& resource_path,
    int* resource_id) const {
  base::FilePath directory_path = extension_path;
  base::FilePath resources_dir;
  base::FilePath relative_path;
  if (!PathService::Get(chrome::DIR_RESOURCES, &resources_dir) ||
      !resources_dir.AppendRelativePath(directory_path, &relative_path)) {
    return false;
  }
  relative_path = relative_path.Append(resource_path);
  relative_path = relative_path.NormalizePathSeparators();

  std::map<base::FilePath, int>::const_iterator entry =
      path_to_resource_id_.find(relative_path);
  if (entry != path_to_resource_id_.end())
    *resource_id = entry->second;

  return entry != path_to_resource_id_.end();
}

bool ChromeComponentExtensionDelegate::GetExtensionStrings(
    content::BrowserContext* browser_context,
    const std::string& extension_id,
    base::DictionaryValue* dict) const {
  if (extension_id != extension_misc::kFeedbackExtensionId)
    return false;

#define SET_STRING(id, idr) dict->SetString(id, l10n_util::GetStringUTF16(idr))
  SET_STRING("page-title", IDS_FEEDBACK_REPORT_PAGE_TITLE);
  SET_STRING("page-title-sad-tab", IDS_FEEDBACK_REPORT_PAGE_TITLE_SAD_TAB_FLOW);
  SET_STRING("additionalInfo", IDS_FEEDBACK_ADDITIONAL_INFO_LABEL);
  SET_STRING("minimize-btn-label", IDS_FEEDBACK_MINIMIZE_BUTTON_LABEL);
  SET_STRING("close-btn-label", IDS_FEEDBACK_CLOSE_BUTTON_LABEL);
  SET_STRING("page-url", IDS_FEEDBACK_REPORT_URL_LABEL);
  SET_STRING("screenshot", IDS_FEEDBACK_SCREENSHOT_LABEL);
  SET_STRING("user-email", IDS_FEEDBACK_USER_EMAIL_LABEL);
  SET_STRING("anonymous-user", IDS_FEEDBACK_ANONYMOUS_EMAIL_OPTION);
#if defined(OS_CHROMEOS)
  if (arc::IsArcPlayStoreEnabledForProfile(
          Profile::FromBrowserContext(browser_context))) {
    SET_STRING("sys-info",
               IDS_FEEDBACK_INCLUDE_SYSTEM_INFORMATION_AND_METRICS_CHKBOX_ARC);
  } else {
    SET_STRING("sys-info",
               IDS_FEEDBACK_INCLUDE_SYSTEM_INFORMATION_AND_METRICS_CHKBOX);
  }
#else
  SET_STRING("sys-info", IDS_FEEDBACK_INCLUDE_SYSTEM_INFORMATION_CHKBOX);
#endif
  SET_STRING("attach-file-label", IDS_FEEDBACK_ATTACH_FILE_LABEL);
  SET_STRING("attach-file-note", IDS_FEEDBACK_ATTACH_FILE_NOTE);
  SET_STRING("attach-file-to-big", IDS_FEEDBACK_ATTACH_FILE_TO_BIG);
  SET_STRING("reading-file", IDS_FEEDBACK_READING_FILE);
  SET_STRING("send-report", IDS_FEEDBACK_SEND_REPORT);
  SET_STRING("cancel", IDS_CANCEL);
  SET_STRING("no-description", IDS_FEEDBACK_NO_DESCRIPTION);
  SET_STRING("privacy-note", IDS_FEEDBACK_PRIVACY_NOTE);
  SET_STRING("performance-trace",
             IDS_FEEDBACK_INCLUDE_PERFORMANCE_TRACE_CHECKBOX);
  // Add the localized strings needed for the "system information" page.
  SET_STRING("sysinfoPageTitle", IDS_FEEDBACK_SYSINFO_PAGE_TITLE);
  SET_STRING("sysinfoPageDescription", IDS_ABOUT_SYS_DESC);
  SET_STRING("sysinfoPageTableTitle", IDS_ABOUT_SYS_TABLE_TITLE);
  SET_STRING("sysinfoPageExpandAllBtn", IDS_ABOUT_SYS_EXPAND_ALL);
  SET_STRING("sysinfoPageCollapseAllBtn", IDS_ABOUT_SYS_COLLAPSE_ALL);
  SET_STRING("sysinfoPageExpandBtn", IDS_ABOUT_SYS_EXPAND);
  SET_STRING("sysinfoPageCollapseBtn", IDS_ABOUT_SYS_COLLAPSE);
  SET_STRING("sysinfoPageStatusLoading", IDS_FEEDBACK_SYSINFO_PAGE_LOADING);
  // And the localized strings needed for the SRT Download Prompt.
  SET_STRING("srtPromptBody", IDS_FEEDBACK_SRT_PROMPT_BODY);
  SET_STRING("srtPromptAcceptButton", IDS_FEEDBACK_SRT_PROMPT_ACCEPT_BUTTON);
  SET_STRING("srtPromptDeclineButton", IDS_FEEDBACK_SRT_PROMPT_DECLINE_BUTTON);
#undef SET_STRING

  webui::SetLoadTimeDataDefaults(g_browser_process->GetApplicationLocale(),
                                 dict);

  return true;
}

void ChromeComponentExtensionDelegate::AddComponentResourceEntries(
    const GritResourceMap* entries,
    size_t size) {
  for (size_t i = 0; i < size; ++i) {
    base::FilePath resource_path = base::FilePath().AppendASCII(
        entries[i].name);
    resource_path = resource_path.NormalizePathSeparators();

    DCHECK(path_to_resource_id_.find(resource_path) ==
           path_to_resource_id_.end());
    path_to_resource_id_[resource_path] = entries[i].value;
  }
}

}  // namespace extensions
