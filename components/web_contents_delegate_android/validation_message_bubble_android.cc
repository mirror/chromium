// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/web_contents_delegate_android/validation_message_bubble_android.h"

#include "base/android/jni_string.h"
#include "content/public/browser/web_contents.h"
#include "jni/ValidationMessageBubble_jni.h"
#include "ui/android/view_android.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/size_conversions.h"

using base::android::ConvertUTF16ToJavaString;
using content::WebContents;

namespace {

gfx::Rect ScaleToRoundedRect(const gfx::Rect& rect, float scale) {
  gfx::RectF scaledRect(rect);
  scaledRect.Scale(scale);
  return ToNearestRect(scaledRect);
}

gfx::Size ScaleToRoundedSize(const gfx::SizeF& size, float scale) {
  return gfx::ToRoundedSize(gfx::ScaleSize(size, scale));
}
}  // namespace

namespace web_contents_delegate_android {

ValidationMessageBubbleAndroid::ValidationMessageBubbleAndroid(
    content::WebContents* web_contents,
    const gfx::Rect& anchor_in_screen,
    const base::string16& main_text,
    const base::string16& sub_text)
    : initialized_(false) {
  Show(web_contents->GetNativeView(), anchor_in_screen, main_text, sub_text);
  initialized_ = true;
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
  if (java_validation_message_bubble_.is_null())
    return;
  base::string16 unused;
  Show(web_contents->GetNativeView(), anchor_in_screen, unused, unused);
}

void ValidationMessageBubbleAndroid::Show(gfx::NativeView view,
                                          const gfx::Rect& anchor_in_screen,
                                          const base::string16& main_text,
                                          const base::string16& sub_text) {
  // Convert to physical pixel unit before passing to Java.
  float scale = view->GetDipScale();
  gfx::Rect anchor = ScaleToRoundedRect(anchor_in_screen, scale);
  gfx::Size viewport = ScaleToRoundedSize(view->viewport_size(), scale);
  JNIEnv* env = base::android::AttachCurrentThread();
  if (!initialized_) {
    java_validation_message_bubble_.Reset(
        Java_ValidationMessageBubble_createAndShowIfApplicable(
            env, view->GetContainerView(), viewport.width(), viewport.height(),
            view->content_offset() * scale, anchor.x(), anchor.y(),
            anchor.width(), anchor.height(),
            ConvertUTF16ToJavaString(env, main_text),
            ConvertUTF16ToJavaString(env, sub_text)));
  } else {
    Java_ValidationMessageBubble_setPositionRelativeToAnchor(
        env, java_validation_message_bubble_, viewport.width(),
        viewport.height(), view->content_offset() * scale, anchor.x(),
        anchor.y(), anchor.width(), anchor.height());
  }
}

}  // namespace web_contents_delegate_android
