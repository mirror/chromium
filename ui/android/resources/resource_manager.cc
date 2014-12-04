// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/android/resources/resource_manager.h"

#include "content/public/browser/android/ui_resource_provider.h"
#include "jni/ResourceManager_jni.h"
#include "ui/android/resources/ui_resource_android.h"
#include "ui/gfx/android/java_bitmap.h"
#include "ui/gfx/geometry/insets_f.h"

namespace ui {

ResourceManager::Resource::Resource() {
}

ResourceManager::Resource::~Resource() {
}

gfx::Rect ResourceManager::Resource::Border(const gfx::Size& bounds) {
  return Border(bounds, gfx::InsetsF(1.f, 1.f, 1.f, 1.f));
}

gfx::Rect ResourceManager::Resource::Border(const gfx::Size& bounds,
                                            const gfx::InsetsF& scale) {
  // Calculate whether or not we need to scale down the border if the bounds of
  // the layer are going to be smaller than the aperture padding.
  float x_scale = std::min((float)bounds.width() / size.width(), 1.f);
  float y_scale = std::min((float)bounds.height() / size.height(), 1.f);

  float left_scale = std::min(x_scale * scale.left(), 1.f);
  float right_scale = std::min(x_scale * scale.right(), 1.f);
  float top_scale = std::min(y_scale * scale.top(), 1.f);
  float bottom_scale = std::min(y_scale * scale.bottom(), 1.f);

  return gfx::Rect(aperture.x() * left_scale, aperture.y() * top_scale,
                   (size.width() - aperture.width()) * right_scale,
                   (size.height() - aperture.height()) * bottom_scale);
}

// static
ResourceManager* ResourceManager::FromJavaObject(jobject jobj) {
  return reinterpret_cast<ResourceManager*>(Java_ResourceManager_getNativePtr(
      base::android::AttachCurrentThread(), jobj));
}

ResourceManager::ResourceManager(
    content::UIResourceProvider* ui_resource_provider)
    : ui_resource_provider_(ui_resource_provider) {
  JNIEnv* env = base::android::AttachCurrentThread();
  java_obj_.Reset(env, Java_ResourceManager_create(
                           env, base::android::GetApplicationContext(),
                           reinterpret_cast<intptr_t>(this)).obj());
  DCHECK(!java_obj_.is_null());
}

ResourceManager::~ResourceManager() {
  Java_ResourceManager_destroy(base::android::AttachCurrentThread(),
                               java_obj_.obj());
}

base::android::ScopedJavaLocalRef<jobject> ResourceManager::GetJavaObject(
    JNIEnv* env) {
  return base::android::ScopedJavaLocalRef<jobject>(java_obj_);
}

ResourceManager::Resource* ResourceManager::GetResource(
    AndroidResourceType res_type,
    int res_id) {
  DCHECK_GE(res_type, ANDROID_RESOURCE_TYPE_FIRST);
  DCHECK_LE(res_type, ANDROID_RESOURCE_TYPE_LAST);

  Resource* resource = resources_[res_type].Lookup(res_id);

  if (!resource || res_type == ANDROID_RESOURCE_TYPE_DYNAMIC ||
      res_type == ANDROID_RESOURCE_TYPE_DYNAMIC_BITMAP) {
    Java_ResourceManager_resourceRequested(base::android::AttachCurrentThread(),
                                           java_obj_.obj(), res_type, res_id);
    resource = resources_[res_type].Lookup(res_id);
  }

  return resource;
}

void ResourceManager::PreloadResource(AndroidResourceType res_type,
                                      int res_id) {
  DCHECK_GE(res_type, ANDROID_RESOURCE_TYPE_FIRST);
  DCHECK_LE(res_type, ANDROID_RESOURCE_TYPE_LAST);

  // Don't send out a query if the resource is already loaded.
  if (resources_[res_type].Lookup(res_id))
    return;

  Java_ResourceManager_preloadResource(base::android::AttachCurrentThread(),
                                       java_obj_.obj(), res_type, res_id);
}

void ResourceManager::OnResourceReady(JNIEnv* env,
                                      jobject jobj,
                                      jint res_type,
                                      jint res_id,
                                      jobject bitmap,
                                      jint padding_left,
                                      jint padding_top,
                                      jint padding_right,
                                      jint padding_bottom,
                                      jint aperture_left,
                                      jint aperture_top,
                                      jint aperture_right,
                                      jint aperture_bottom) {
  DCHECK_GE(res_type, ANDROID_RESOURCE_TYPE_FIRST);
  DCHECK_LE(res_type, ANDROID_RESOURCE_TYPE_LAST);

  Resource* resource = resources_[res_type].Lookup(res_id);
  if (!resource) {
    resource = new Resource();
    resources_[res_type].AddWithID(resource, res_id);
  }

  gfx::JavaBitmap jbitmap(bitmap);
  resource->size = jbitmap.size();
  resource->padding.SetRect(padding_left, padding_top,
                            padding_right - padding_left,
                            padding_bottom - padding_top);
  resource->aperture.SetRect(aperture_left, aperture_top,
                             aperture_right - aperture_left,
                             aperture_bottom - aperture_top);
  resource->ui_resource =
      UIResourceAndroid::CreateFromJavaBitmap(ui_resource_provider_, jbitmap);
}

// static
bool ResourceManager::RegisterResourceManager(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace ui
