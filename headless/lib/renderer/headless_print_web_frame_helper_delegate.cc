// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/lib/renderer/headless_print_web_frame_helper_delegate.h"

#include "build/build_config.h"
#include "third_party/WebKit/public/web/WebElement.h"

namespace headless {

HeadlessPrintWebFrameHelperDelegate::HeadlessPrintWebFrameHelperDelegate() {}

HeadlessPrintWebFrameHelperDelegate::~HeadlessPrintWebFrameHelperDelegate() {}

bool HeadlessPrintWebFrameHelperDelegate::CancelPrerender(
    content::RenderFrame* render_frame) {
  return false;
}

blink::WebElement HeadlessPrintWebFrameHelperDelegate::GetPdfElement(
    blink::WebLocalFrame* frame) {
  return blink::WebElement();
}

bool HeadlessPrintWebFrameHelperDelegate::IsPrintPreviewEnabled() {
  return false;
}

bool HeadlessPrintWebFrameHelperDelegate::OverridePrint(
    blink::WebLocalFrame* frame) {
  return false;
}

bool HeadlessPrintWebFrameHelperDelegate::IsAskPrintSettingsEnabled() {
  return true;
}

#if defined(OS_MACOSX)
bool HeadlessPrintWebFrameHelperDelegate::UseSingleMetafile() {
  return true;
}
#endif

}  // namespace headless
