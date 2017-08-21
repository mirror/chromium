// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/embedder/parcelable_channel.h"

#include "base/android/jni_android.h"
#include "mojo/edk/system/android/parcelable_channel_client_impl.h"

using base::android::ScopedJavaLocalRef;

namespace mojo {
namespace edk {

// static
ParcelableChannel ParcelableChannel::CreateForParentProcess() {
  ParcelableChannel channel;
  channel.client_channel_proxy_ =
      std::make_unique<ParcelableChannelClientProxy>();
  channel.server_channel_ = std::make_unique<ParcelableChannelServer>();
  return channel;
}

// static
ParcelableChannel ParcelableChannel::CreateForChildProcess(
    const base::android::JavaRef<jobjectArray>& ibinders) {
  JNIEnv* env = base::android::AttachCurrentThread();
  CHECK_GE(env->GetArrayLength(ibinders.obj()), 2);
  ScopedJavaLocalRef<jobject> iclient_channel(
      env, env->GetObjectArrayElement(ibinders.obj(), 0));
  base::android::ClearException(env);

  ScopedJavaLocalRef<jobject> ichannel_provider(
      env, env->GetObjectArrayElement(ibinders.obj(), 1));
  base::android::ClearException(env);

  ParcelableChannel channel;
  channel.client_channel_ =
      std::make_unique<ParcelableChannelClientImpl>(iclient_channel);
  channel.server_channel_ = std::make_unique<ParcelableChannelServer>();
  // Make the server channel available on the client.
  channel.server_channel_->SetOnChannelProvider(ichannel_provider);
  return channel;
}

ParcelableChannel::ParcelableChannel() {}

ParcelableChannel::~ParcelableChannel() {}

ParcelableChannel::ParcelableChannel(ParcelableChannel&& other) = default;

ParcelableChannel& ParcelableChannel::operator=(ParcelableChannel&& other) =
    default;

ScopedJavaLocalRef<jobjectArray>
ParcelableChannel::GetIBindersForClientProcess() {
  DCHECK(server_channel_);
  DCHECK(client_channel_proxy_);

  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jclass> ibinder_clazz =
      base::android::GetClass(env, "android/os/IBinder");
  ScopedJavaLocalRef<jobjectArray> ibinders(
      env, env->NewObjectArray(2, ibinder_clazz.obj(), NULL));
  base::android::CheckException(env);

  env->SetObjectArrayElement(ibinders.obj(), 0,
                             server_channel_->GetIParcelableChannel().obj());
  base::android::CheckException(env);
  env->SetObjectArrayElement(
      ibinders.obj(), 1,
      client_channel_proxy_->GetIParcelableChannelProvider().obj());
  base::android::CheckException(env);

  return ibinders;
}

std::unique_ptr<ParcelableChannelClient>
ParcelableChannel::TakeParcelableChannelClient() {
  return client_channel_ ? std::move(client_channel_)
                         : std::move(client_channel_proxy_);
}

std::unique_ptr<ParcelableChannelServer>
ParcelableChannel::TakeParcelableChannelServer() {
  return std::move(server_channel_);
}

}  // namespace edk
}  // namespace mojo
