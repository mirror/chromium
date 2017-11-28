// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/win/photo_callback_factory_mf_win.h"

#include <mfapi.h>
#include <wrl/client.h>

#include "base/memory/ref_counted.h"
#include "media/capture/video/blob_utils.h"

using Microsoft::WRL::ComPtr;

namespace media {

namespace {
class MFPhotoCallback final
    : public base::RefCountedThreadSafe<MFPhotoCallback>,
      public IMFCaptureEngineOnSampleCallback {
 public:
  MFPhotoCallback(media::VideoCaptureDevice::TakePhotoCallback callback,
                  VideoCaptureFormat format)
      : callback_(std::move(callback)), format_(format) {}

  STDMETHOD(QueryInterface)(REFIID riid, void** object) override {
    if (riid == IID_IUnknown || riid == IID_IMFCaptureEngineOnSampleCallback) {
      AddRef();
      *object = static_cast<IMFCaptureEngineOnSampleCallback*>(this);
      return S_OK;
    }
    return E_NOINTERFACE;
  }

  STDMETHOD_(ULONG, AddRef)() override {
    base::RefCountedThreadSafe<MFPhotoCallback>::AddRef();
    return 1U;
  }

  STDMETHOD_(ULONG, Release)() override {
    base::RefCountedThreadSafe<MFPhotoCallback>::Release();
    return 1U;
  }

  STDMETHOD(OnSample)(IMFSample* sample) override {
    if (!sample)
      return S_OK;

    DWORD buffer_count = 0;
    sample->GetBufferCount(&buffer_count);

    for (DWORD i = 0; i < buffer_count; ++i) {
      ComPtr<IMFMediaBuffer> buffer;
      sample->GetBufferByIndex(i, buffer.GetAddressOf());
      if (!buffer.Get()) {
        continue;
      }
      DWORD length = 0;
      DWORD max_length = 0;
      BYTE* data = nullptr;
      buffer->Lock(&data, &max_length, &length);
      mojom::BlobPtr blob = Blobify(data, length, format_);
      buffer->Unlock();
      if (blob) {
        std::move(callback_).Run(std::move(blob));
        // What it is supposed to mean if there is more than buffer sent to us
        // as a response to requesting a single still image.
        // Are we supposed to somehow concatenate the buffers? Or is it safe
        // to ignore extra buffers? For now, we ignore extra buffers.
        break;
      }
    }
    return S_OK;
  }

 private:
  friend class base::RefCountedThreadSafe<MFPhotoCallback>;
  ~MFPhotoCallback() {}

  media::VideoCaptureDevice::TakePhotoCallback callback_;
  const VideoCaptureFormat format_;
};
}  // namespace

MFPhotoCallbackFactory::MFPhotoCallbackFactory() {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

scoped_refptr<IMFCaptureEngineOnSampleCallback>
MFPhotoCallbackFactory::CreateCallback(
    media::VideoCaptureDevice::TakePhotoCallback callback,
    VideoCaptureFormat format) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return scoped_refptr<IMFCaptureEngineOnSampleCallback>(
      new MFPhotoCallback(std::move(callback), std::move(format)));
}

MFPhotoCallbackFactory::~MFPhotoCallbackFactory() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

}  // namespace media