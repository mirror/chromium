// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/web_contents_delegate_android/validation_message_bubble_android.h"

#include "base/android/jni_string.h"
#include "content/public/browser/web_contents.h"
#include "jni/ValidationMessageBubble_jni.h"
#include "ui/android/view_android.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/size_conversions.h"

using base::android::ConvertUTF16ToJavaString;
using content::WebContents;

namespace web_contents_delegate_android {
namespace {
gfx::Rect ScaleToRoundedRect(const gfx::Rect& rect, float scale) {
  gfx::RectF scaledRect(rect);
  scaledRect.Scale(scale);
  return gfx::Rect(gfx::ToRoundedInt(scaledRect.x()),
                   gfx::ToRoundedInt(scaledRect.y()),
                   gfx::ToRoundedInt(scaledRect.width()),
                   gfx::ToRoundedInt(scaledRect.height()));
}

gfx::Size ScaleToRoundedSize(const gfx::SizeF& size, float scale) {
  return gfx::ToRoundedSize(gfx::ScaleSize(size, scale));
}
}  // namespace

ValidationMessageBubbleAndroid::ValidationMessageBubbleAndroid(
    content::WebContents* web_contents,
    const gfx::Rect& anchor_in_screen,
    const base::string16& main_text,
    const base::string16& sub_text) {
  gfx::NativeView view = web_contents->GetNativeView();
  float scale = view->GetDipScale();
  // Convert to device pixel unit before passing to Java.
  auto anchor = ScaleToRoundedRect(anchor_in_screen, scale);
  auto viewport = ScaleToRoundedSize(view->GetViewportSize(), scale);

  JNIEnv* env = base::android::AttachCurrentThread();
  java_validation_message_bubble_.Reset(
      Java_ValidationMessageBubble_createAndShowIfApplicable(
          env, view->GetContainerView(), viewport.width(), viewport.height(),
          view->content_offset() * scale, anchor.x(), anchor.y(),
          anchor.width(), anchor.height(),
          ConvertUTF16ToJavaString(env, main_text),
          ConvertUTF16ToJavaString(env, sub_text)));
}

ValidationMessageBubbleAndroid::~ValidationMessageBubbleAndroid() {
  if (!java_validation_message_bubble_.is_null()) {
    Java_ValidationMessageBubble_close(base::android::AttachCurrentThread(),
                                       java_validation_message_bubble_);
  }
}

void ValidationMessageBubbleAndroid::SetPositionRelativeToAnchor(
    WebContents* web_contents,
    const gfx::Rect& anchor_in_screen) {
  gfx::NativeView view = web_contents->GetNativeView();
  float scale = view->GetDipScale();
  // Convert to device pixel unit before passing to Java.
  auto anchor = ScaleToRoundedRect(anchor_in_screen, scale);
  auto viewport = ScaleToRoundedSize(view->GetViewportSize(), scale);

  Java_ValidationMessageBubble_setPositionRelativeToAnchor(
      base::android::AttachCurrentThread(), java_validation_message_bubble_,
      view->GetContainerView(), viewport.width(), viewport.height(),
      view->content_offset() * scale, anchor.x(), anchor.y(), anchor.width(),
      anchor.height());
}

}  // namespace web_contents_delegate_android
