// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/ptr_util.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/views/permission_bubble/permission_prompt_impl.h"
#include "chrome/browser/ui/views_mode_controller.h"

// static
std::unique_ptr<PermissionPrompt> PermissionPrompt::Create(
    content::WebContents* web_contents,
    Delegate* delegate) {
#if defined(OS_MACOSX)
  if (views_mode_controller::IsViewsBrowserCocoa())
    return PermissionPrompt::CreateForCocoaWindow(web_contents, delegate);
#endif
  return base::WrapUnique(new PermissionPromptImpl(
      chrome::FindBrowserWithWebContents(web_contents), delegate));
}
