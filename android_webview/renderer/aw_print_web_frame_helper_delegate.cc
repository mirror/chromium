// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/renderer/aw_print_web_frame_helper_delegate.h"

#include "third_party/WebKit/public/web/WebElement.h"

namespace android_webview {

AwPrintWebFrameHelperDelegate::~AwPrintWebFrameHelperDelegate() {}

bool AwPrintWebFrameHelperDelegate::CancelPrerender(
    content::RenderFrame* render_frame) {
  return false;
}

blink::WebElement AwPrintWebFrameHelperDelegate::GetPdfElement(
    blink::WebLocalFrame* frame) {
  return blink::WebElement();
}

bool AwPrintWebFrameHelperDelegate::IsPrintPreviewEnabled() {
  return false;
}

bool AwPrintWebFrameHelperDelegate::IsScriptedPrintEnabled() {
  return false;
}

bool AwPrintWebFrameHelperDelegate::OverridePrint(blink::WebLocalFrame* frame) {
  return false;
}

}  // namespace android_webview
