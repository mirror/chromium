// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/dialog_overlay_impl.h"

#include "content/public/browser/web_contents.h"
#include "gpu/ipc/common/gpu_surface_tracker.h"
#include "jni/DialogOverlayImpl_jni.h"
#include "ui/android/window_android.h"

using base::android::AttachCurrentThread;
using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

namespace content {

// static
bool DialogOverlayImpl::RegisterDialogOverlayImpl(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

static jlong Init(JNIEnv* env,
                  const JavaParamRef<jobject>& obj,
                  jlong high,
                  jlong low) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  RenderFrameHostImpl* rfhi =
      content::RenderFrameHostImpl::FromOverlayRoutingToken(
          base::UnguessableToken::Deserialize(high, low));

  if (!rfhi)
    return 0;

  WebContentsImpl* web_contents_impl = static_cast<WebContentsImpl*>(
      content::WebContents::FromRenderFrameHost(rfhi));

  // If the overlay would not be immediately used, fail the request.
  if (!rfhi->IsCurrent() || web_contents_impl->IsHidden())
    return 0;

  ContentViewCoreImpl* cvc =
      content::ContentViewCoreImpl::FromWebContents(web_contents_impl);

  if (!cvc)
    return 0;

  return reinterpret_cast<jlong>(
      new DialogOverlayImpl(obj, rfhi, web_contents_impl, cvc));
}

DialogOverlayImpl::DialogOverlayImpl(const JavaParamRef<jobject>& obj,
                                     RenderFrameHostImpl* rfhi,
                                     WebContents* web_contents,
                                     ContentViewCoreImpl* cvc)
    : WebContentsObserver(web_contents), rfhi_(rfhi), cvc_(cvc) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  DCHECK(rfhi_);
  DCHECK(cvc_);

  JNIEnv* env = AttachCurrentThread();
  obj_ = JavaObjectWeakGlobalRef(env, obj);

  cvc_->AddObserver(this);

  // Also send the initial token, since we'll only get changes.
  if (ui::WindowAndroid* window = cvc_->GetWindowAndroid()) {
    ScopedJavaLocalRef<jobject> token = window->GetWindowToken();
    if (!token.is_null())
      Java_DialogOverlayImpl_onWindowToken(env, obj.obj(), token);
  }
}

DialogOverlayImpl::~DialogOverlayImpl() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // We should only be deleted after one unregisters for token callbacks.
  DCHECK(!cvc_);
}

void DialogOverlayImpl::Stop() {
  rfhi_ = nullptr;
  UnregisterForTokensIfNeeded();

  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = obj_.get(env);
  if (!obj.is_null())
    Java_DialogOverlayImpl_onDismissed(env, obj.obj());
}

void DialogOverlayImpl::Destroy(JNIEnv* env, const JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  UnregisterForTokensIfNeeded();
  // We delete soon since this might be part of an onDismissed callback.
  BrowserThread::DeleteSoon(BrowserThread::UI, FROM_HERE, this);
}

void DialogOverlayImpl::GetCompositorOffset(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    const base::android::JavaParamRef<jobject>& rect) {
  gfx::Point point;
  if (cvc_) {
    if (ui::ViewAndroid* view = cvc_->GetViewAndroid())
      point = view->GetLocationOfContainerViewOnScreen();
  }

  Java_DialogOverlayImpl_receiveCompositorOffset(env, obj.obj(), rect.obj(),
                                                 point.x(), point.y());
}

void DialogOverlayImpl::UnregisterForTokensIfNeeded() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!cvc_)
    return;

  cvc_->RemoveObserver(this);
  cvc_ = nullptr;
}

void DialogOverlayImpl::OnContentViewCoreDestroyed() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  Stop();
}

void DialogOverlayImpl::RenderFrameDeleted(RenderFrameHost* render_frame_host) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (render_frame_host == rfhi_)
    Stop();
}

void DialogOverlayImpl::RenderFrameHostChanged(RenderFrameHost* old_host,
                                               RenderFrameHost* new_host) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (old_host == rfhi_)
    Stop();
}

void DialogOverlayImpl::FrameDeleted(RenderFrameHost* render_frame_host) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (render_frame_host == rfhi_)
    Stop();
}

void DialogOverlayImpl::WasHidden() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  Stop();
}

void DialogOverlayImpl::WebContentsDestroyed() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  Stop();
}

void DialogOverlayImpl::OnAttachedToWindow() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jobject> token;

  if (ui::WindowAndroid* window = cvc_->GetWindowAndroid())
    token = window->GetWindowToken();

  ScopedJavaLocalRef<jobject> obj = obj_.get(env);
  if (!obj.is_null())
    Java_DialogOverlayImpl_onWindowToken(env, obj.obj(), token);
}

void DialogOverlayImpl::OnDetachedFromWindow() {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = obj_.get(env);
  if (!obj.is_null())
    Java_DialogOverlayImpl_onWindowToken(env, obj.obj(), nullptr);
}

static jint RegisterSurface(JNIEnv* env,
                            const base::android::JavaParamRef<jclass>& jcaller,
                            const JavaParamRef<jobject>& surface) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return gpu::GpuSurfaceTracker::Get()->AddSurfaceForNativeWidget(
      gpu::GpuSurfaceTracker::SurfaceRecord(gfx::kNullAcceleratedWidget,
                                            surface.obj()));
}

static void UnregisterSurface(
    JNIEnv* env,
    const base::android::JavaParamRef<jclass>& jcaller,
    jint surface_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  gpu::GpuSurfaceTracker::Get()->RemoveSurface(surface_id);
}

}  // namespace content
