// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/renderer_host/chrome_navigation_ui_data.h"

#include <tuple>

#include "base/memory/ptr_util.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "content/public/browser/navigation_handle.h"
#include "extensions/features/features.h"
#include "ui/base/window_open_disposition.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
namespace {
std::pair<int, int> GetTabIdAndWindowId(content::WebContents* web_contents) {
  SessionTabHelper* session_tab_helper =
      SessionTabHelper::FromWebContents(web_contents);
  if (!session_tab_helper)
    return std::make_pair(-1, -1);

  return std::make_pair(session_tab_helper->session_id().id(),
                        session_tab_helper->window_id().id());
}
}  // namespace
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

ChromeNavigationUIData::ChromeNavigationUIData() {}

ChromeNavigationUIData::ChromeNavigationUIData(
    content::NavigationHandle* navigation_handle)
    : disposition_(WindowOpenDisposition::CURRENT_TAB) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  int tab_id;
  int window_id;
  std::tie(tab_id, window_id) =
      GetTabIdAndWindowId(navigation_handle->GetWebContents());
  extension_data_ = base::MakeUnique<extensions::ExtensionNavigationUIData>(
      navigation_handle, tab_id, window_id);
#endif
}

ChromeNavigationUIData::~ChromeNavigationUIData() {}

// static
std::unique_ptr<ChromeNavigationUIData>
ChromeNavigationUIData::CreateForMainFrameNavigation(
    content::WebContents* web_contents,
    WindowOpenDisposition disposition) {
  return base::WrapUnique(
      new ChromeNavigationUIData(web_contents, disposition));
}

std::unique_ptr<content::NavigationUIData> ChromeNavigationUIData::Clone()
    const {
  std::unique_ptr<ChromeNavigationUIData> copy =
      base::MakeUnique<ChromeNavigationUIData>();

  copy->disposition_ = disposition_;

#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (extension_data_)
    copy->SetExtensionNavigationUIData(extension_data_->DeepCopy());
#endif

#if BUILDFLAG(ENABLE_OFFLINE_PAGES)
  if (offline_page_data_)
    copy->SetOfflinePageNavigationUIData(offline_page_data_->DeepCopy());
#endif

  return std::move(copy);
}

#if BUILDFLAG(ENABLE_EXTENSIONS)
void ChromeNavigationUIData::SetExtensionNavigationUIData(
    std::unique_ptr<extensions::ExtensionNavigationUIData> extension_data) {
  extension_data_ = std::move(extension_data);
}
#endif

#if BUILDFLAG(ENABLE_OFFLINE_PAGES)
void ChromeNavigationUIData::SetOfflinePageNavigationUIData(
    std::unique_ptr<offline_pages::OfflinePageNavigationUIData>
        offline_page_data) {
  offline_page_data_ = std::move(offline_page_data);
}
#endif

ChromeNavigationUIData::ChromeNavigationUIData(
    content::WebContents* web_contents,
    WindowOpenDisposition disposition)
    : disposition_(disposition) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  int tab_id;
  int window_id;
  std::tie(tab_id, window_id) = GetTabIdAndWindowId(web_contents);
  extension_data_ =
      extensions::ExtensionNavigationUIData::CreateForMainFrameNavigation(
          web_contents, tab_id, window_id);
#endif
}
