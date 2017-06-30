// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/motion_event_synthesizer_impl.h"

#include "base/memory/ptr_util.h"
#include "content/browser/web_contents/web_contents_android.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "jni/MotionEventSynthesizer_jni.h"
#include "ui/android/view_android.h"

namespace content {

// static
std::unique_ptr<MotionEventSynthesizer> MotionEventSynthesizer::Create(
    Helper* helper,
    ui::ViewAndroid* view) {
  return base::WrapUnique(new MotionEventSynthesizerImpl(helper, view));
}

MotionEventSynthesizerImpl::MotionEventSynthesizerImpl(Helper* helper,
                                                       ui::ViewAndroid* view)
    : scale_factor_(view->GetDipScale()), helper_(helper) {
  // |view| should not be owned or stored in this class.
  DCHECK(helper);
  auto* web_contents = static_cast<WebContentsImpl*>(helper->GetWebContents());
  JNIEnv* env = base::android::AttachCurrentThread();
  web_contents->GetWebContentsAndroid()->SetMotionEventSynthesizer(
      Java_MotionEventSynthesizer_create(env, view->GetContainerView()));
}

MotionEventSynthesizerImpl::~MotionEventSynthesizerImpl() {}

base::android::ScopedJavaLocalRef<jobject>
MotionEventSynthesizerImpl::GetJavaObject() {
  auto* web_contents = static_cast<WebContentsImpl*>(helper_->GetWebContents());
  return web_contents->GetWebContentsAndroid()->GetMotionEventSynthesizer();
}

void MotionEventSynthesizerImpl::SetPointer(int index, int x, int y, int id) {
  base::android::ScopedJavaLocalRef<jobject> obj = GetJavaObject();
  if (obj.is_null())
    return;

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_MotionEventSynthesizer_setPointer(env, obj, index, x * scale_factor_,
                                         y * scale_factor_, id);
}

void MotionEventSynthesizerImpl::SetScrollDeltas(int x, int y, int dx, int dy) {
  base::android::ScopedJavaLocalRef<jobject> obj = GetJavaObject();
  if (obj.is_null())
    return;

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_MotionEventSynthesizer_setScrollDeltas(
      env, obj, x * scale_factor_, y * scale_factor_, dx * scale_factor_,
      dy * scale_factor_);
}

void MotionEventSynthesizerImpl::Inject(MotionEventAction action,
                                        int pointer_count,
                                        int64_t time_ms) {
  base::android::ScopedJavaLocalRef<jobject> obj = GetJavaObject();
  if (obj.is_null())
    return;

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_MotionEventSynthesizer_inject(env, obj, action, pointer_count, time_ms);
}

}  // namespace content
