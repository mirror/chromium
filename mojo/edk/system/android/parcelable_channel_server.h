// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_SYSTEM_PARCELABLE_CHANNEL_SERVER_H_
#define MOJO_EDK_SYSTEM_PARCELABLE_CHANNEL_SERVER_H_

#include "base/android/scoped_java_ref.h"

namespace mojo {
namespace edk {

class ParcelableChannelServer {
 public:
  // Listener to get notified when a Pracelable is received by this
  // ParcelableChannelServer.
  class Listener {
   public:
    // IMPORTANT NOTE: this is called on a background thread.
    virtual void OnParcelableReceived(
        uint32_t id,
        const base::android::JavaRef<jobject>& parcelable) = 0;

   protected:
    virtual ~Listener() {}
  };

  ParcelableChannelServer();
  ~ParcelableChannelServer();

  ParcelableChannelServer(ParcelableChannelServer&& other);
  ParcelableChannelServer& operator=(ParcelableChannelServer&& other);

  void SetListener(Listener* listener);

  bool is_valid() const { return !j_parcelable_channel_.is_null(); }

  base::android::ScopedJavaGlobalRef<jobject> GetIParcelableChannel();

  bool SetOnChannelProvider(
      const base::android::JavaRef<jobject>& parcelableChannelProvider);

  void OnParcelableReceived(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& jcaller,
      jint id,
      const base::android::JavaParamRef<jobject>& parcelable);

 private:
  base::android::ScopedJavaGlobalRef<jobject> j_parcelable_channel_;

  Listener* listener_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(ParcelableChannelServer);
};

}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_SYSTEM_PARCELABLE_CHANNEL_SERVER_H_