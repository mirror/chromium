// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/ptr_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/bubble_anchor_util.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/location_bar/location_bar_view_mac.h"
#import "chrome/browser/ui/cocoa/permission_bubble/permission_bubble_cocoa.h"
#import "chrome/browser/ui/cocoa/permission_bubble/permission_bubble_controller.h"
#include "chrome/browser/ui/views/permission_bubble/permission_prompt_impl.h"
#import "ui/base/cocoa/cocoa_base_utils.h"
#include "ui/base/material_design/material_design_controller.h"
#import "ui/gfx/mac/coordinate_conversion.h"

// static
std::unique_ptr<PermissionPrompt> PermissionPrompt::Create(
    content::WebContents* web_contents) {
  if (ui::MaterialDesignController::IsSecondaryUiMaterial()) {
    return base::WrapUnique(new PermissionPromptImpl(
        chrome::FindBrowserWithWebContents(web_contents)));
  }
  return base::MakeUnique<PermissionBubbleCocoa>(
      chrome::FindBrowserWithWebContents(web_contents));
}
