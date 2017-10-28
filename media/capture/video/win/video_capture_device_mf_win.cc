// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/win/video_capture_device_mf_win.h"

#include <mfapi.h>
#include <mferror.h>
#include <stddef.h>
#include <wincodec.h>

#include <thread>
#include <utility>

#include "base/location.h"
#include "base/memory/ref_counted.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "base/synchronization/waitable_event.h"
#include "base/win/scoped_co_mem.h"
#include "base/win/windows_version.h"
#include "media/capture/video/blob_utils.h"
#include "media/capture/video/win/capability_list_win.h"
#include "media/capture/video/win/sink_filter_win.h"

using base::win::ScopedCoMem;
using Microsoft::WRL::ComPtr;
using base::Location;

namespace media {

void LogError(const Location& from_here, HRESULT hr) {
  const std::string log_message = base::StringPrintf(
      "error@ %s, %s, OS message: %s", from_here.ToString().c_str(),
      base::StringPrintf("VideoCaptureDeviceMFWin: %s",
                         logging::SystemErrorCodeToString(hr).c_str())
          .c_str(),
      logging::SystemErrorCodeToString(logging::GetLastSystemErrorCode())
          .c_str());
  DLOG(ERROR) << log_message;
}

static bool GetFrameSize(IMFMediaType* type, gfx::Size* frame_size) {
  UINT32 width32, height32;
  if (FAILED(MFGetAttributeSize(type, MF_MT_FRAME_SIZE, &width32, &height32)))
    return false;
  frame_size->SetSize(width32, height32);
  return true;
}

static bool GetFrameRate(IMFMediaType* type, float* frame_rate) {
  UINT32 numerator, denominator;
  if (FAILED(MFGetAttributeRatio(type, MF_MT_FRAME_RATE, &numerator,
                                 &denominator)) ||
      !denominator) {
    return false;
  }
  *frame_rate = static_cast<float>(numerator) / denominator;
  return true;
}

static bool FillFormat(IMFMediaType* type, VideoCaptureFormat* format) {
  GUID majorTypeGuid;
  if (FAILED(type->GetGUID(MF_MT_MAJOR_TYPE, &majorTypeGuid)) ||
      (majorTypeGuid != MFMediaType_Image &&
       !GetFrameRate(type, &format->frame_rate))) {
    return false;
  }

  GUID subTypeGuid;
  if (FAILED(type->GetGUID(MF_MT_SUBTYPE, &subTypeGuid)) ||
      !GetFrameSize(type, &format->frame_size) ||
      !VideoCaptureDeviceMFWin::FormatFromGuid(subTypeGuid,
                                               &format->pixel_format)) {
    return false;
  }

  return true;
}

static bool CopyAttribute(IMFAttributes* sourceAttributes,
                          IMFAttributes* destinationAttributes,
                          const GUID& key) {
  PROPVARIANT var;
  PropVariantInit(&var);
  HRESULT hr = sourceAttributes->GetItem(key, &var);
  if (SUCCEEDED(hr)) {
    hr = destinationAttributes->SetItem(key, var);
    PropVariantClear(&var);
    if (SUCCEEDED(hr)) {
      return true;
    }
  }
  return false;
}

static bool ConvertToPhotoMediaType(IMFMediaType* sourceMediaType,
                                    IMFMediaType* destinationMediaType) {
  HRESULT hr = S_OK;

  hr = destinationMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Image);
  if (FAILED(hr)) {
    return false;
  }

  hr = destinationMediaType->SetGUID(MF_MT_SUBTYPE, GUID_ContainerFormatJpeg);
  if (FAILED(hr)) {
    return false;
  }

  return CopyAttribute(sourceMediaType, destinationMediaType, MF_MT_FRAME_SIZE);
}

HRESULT GetAvailableDeviceMediaType(IMFCaptureSource* source,
                                    DWORD streamIndex,
                                    DWORD media_type_index,
                                    IMFMediaType** type) {
  HRESULT hr;
  /*
   * Rarely, for some unknown reason, GetAvailableDeviceMediaType returns an
   * undocumented MF_E_INVALIDREQUEST. Retrying solves the issue.
   */
  while (MF_E_INVALIDREQUEST == (hr = source->GetAvailableDeviceMediaType(
                                     streamIndex, media_type_index, type))) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  return hr;
}

HRESULT FillCapabilities(DWORD stream,
                         IMFCaptureSource* source,
                         CapabilityList* capabilities) {
  DWORD media_type_index = 0;
  ComPtr<IMFMediaType> type;
  HRESULT hr;

  while (
      SUCCEEDED(hr = GetAvailableDeviceMediaType(
                    source, stream, media_type_index, type.GetAddressOf()))) {
    VideoCaptureFormat format;
    if (FillFormat(type.Get(), &format))
      capabilities->emplace_back(media_type_index, format);
    type.Reset();
    ++media_type_index;
  }

  if (capabilities->empty() && (SUCCEEDED(hr) || hr == MF_E_NO_MORE_TYPES))
    hr = HRESULT_FROM_WIN32(ERROR_EMPTY);

  return (hr == MF_E_NO_MORE_TYPES) ? S_OK : hr;
}

class MFVideoCallback final
    : public base::RefCountedThreadSafe<MFVideoCallback>,
      public IMFCaptureEngineOnSampleCallback,
      public IMFCaptureEngineOnEventCallback {
 public:
  MFVideoCallback(VideoCaptureDeviceMFWin* observer) : observer_(observer) {}

  STDMETHOD(QueryInterface)(REFIID riid, void** object) override {
    HRESULT hr = E_NOINTERFACE;
    if (riid == IID_IUnknown) {
      *object = static_cast<IMFCaptureEngineOnSampleCallback*>(this);
      hr = S_OK;
    } else if (riid == IID_IMFCaptureEngineOnSampleCallback) {
      *object = static_cast<IMFCaptureEngineOnSampleCallback*>(this);
      hr = S_OK;
    } else if (riid == IID_IMFCaptureEngineOnEventCallback) {
      *object = static_cast<IMFCaptureEngineOnEventCallback*>(this);
      hr = S_OK;
    }
    if (SUCCEEDED(hr)) {
      AddRef();
    }
    return hr;
  }

  STDMETHOD_(ULONG, AddRef)() override {
    base::RefCountedThreadSafe<MFVideoCallback>::AddRef();
    return 1U;
  }

  STDMETHOD_(ULONG, Release)() override {
    base::RefCountedThreadSafe<MFVideoCallback>::Release();
    return 1U;
  }

  STDMETHOD(OnEvent)(IMFMediaEvent* mediaEvent) override {
    observer_->OnEvent(mediaEvent);
    return S_OK;
  }

  STDMETHOD(OnSample)
  (IMFSample* sample) override {
    base::TimeTicks reference_time(base::TimeTicks::Now());

    LONGLONG raw_time_stamp;
    sample->GetSampleTime(&raw_time_stamp);
    base::TimeDelta timestamp =
        base::TimeDelta::FromMicroseconds(raw_time_stamp / 10);
    if (!sample) {
      observer_->OnIncomingCapturedData(NULL, 0, 0, reference_time, timestamp);
      return S_OK;
    }

    DWORD count = 0;
    sample->GetBufferCount(&count);

    for (DWORD i = 0; i < count; ++i) {
      ComPtr<IMFMediaBuffer> buffer;
      sample->GetBufferByIndex(i, buffer.GetAddressOf());
      if (buffer.Get()) {
        DWORD length = 0, max_length = 0;
        BYTE* data = NULL;
        buffer->Lock(&data, &max_length, &length);
        observer_->OnIncomingCapturedData(data, length, 0, reference_time,
                                          timestamp);
        buffer->Unlock();
      }
    }
    return S_OK;
  }

 private:
  friend class base::RefCountedThreadSafe<MFVideoCallback>;
  ~MFVideoCallback() {}
  VideoCaptureDeviceMFWin* observer_;
};

class MFPhotoCallback final
    : public base::RefCountedThreadSafe<MFPhotoCallback>,
      public IMFCaptureEngineOnSampleCallback {
 public:
  MFPhotoCallback(media::VideoCaptureDevice::TakePhotoCallback callback,
                  VideoCaptureFormat format)
      : callback_(std::move(callback)), format_(format) {}

  STDMETHOD(QueryInterface)(REFIID riid, void** object) override {
    HRESULT hr = E_NOINTERFACE;
    if (riid == IID_IUnknown) {
      *object = static_cast<IMFCaptureEngineOnSampleCallback*>(this);
      hr = S_OK;
    } else if (riid == IID_IMFCaptureEngineOnSampleCallback) {
      *object = static_cast<IMFCaptureEngineOnSampleCallback*>(this);
      hr = S_OK;
    }
    if (SUCCEEDED(hr)) {
      AddRef();
    }
    return hr;
  }

  STDMETHOD_(ULONG, AddRef)() override {
    base::RefCountedThreadSafe<MFPhotoCallback>::AddRef();
    return 1U;
  }

  STDMETHOD_(ULONG, Release)() override {
    base::RefCountedThreadSafe<MFPhotoCallback>::Release();
    return 1U;
  }

  STDMETHOD(OnSample)
  (IMFSample* sample) override {
    if (!sample) {
      return S_OK;
    }

    DWORD count = 0;
    sample->GetBufferCount(&count);

    for (DWORD i = 0; i < count; ++i) {
      ComPtr<IMFMediaBuffer> buffer;
      sample->GetBufferByIndex(i, buffer.GetAddressOf());
      if (buffer.Get()) {
        DWORD length = 0, max_length = 0;
        BYTE* data = NULL;
        buffer->Lock(&data, &max_length, &length);
        mojom::BlobPtr blob = Blobify(data, length, format_);
        buffer->Unlock();
        if (blob) {
          std::move(callback_).Run(std::move(blob));
        }
      }
    }
    return S_OK;
  }

 private:
  friend class base::RefCountedThreadSafe<MFPhotoCallback>;
  ~MFPhotoCallback() {}

  media::VideoCaptureDevice::TakePhotoCallback callback_;
  VideoCaptureFormat format_;
};

// static
bool VideoCaptureDeviceMFWin::FormatFromGuid(const GUID& guid,
                                             VideoPixelFormat* format) {
  struct {
    const GUID& guid;
    const VideoPixelFormat format;
  } static const kFormatMap[] = {
      {MFVideoFormat_I420, PIXEL_FORMAT_I420},
      {MFVideoFormat_YUY2, PIXEL_FORMAT_YUY2},
      {MFVideoFormat_UYVY, PIXEL_FORMAT_UYVY},
      {MFVideoFormat_RGB24, PIXEL_FORMAT_RGB24},
      {MFVideoFormat_ARGB32, PIXEL_FORMAT_ARGB},
      {MFVideoFormat_MJPG, PIXEL_FORMAT_MJPEG},
      {GUID_ContainerFormatJpeg, PIXEL_FORMAT_MJPEG},
      {MFVideoFormat_YV12, PIXEL_FORMAT_YV12},
      {kMediaSubTypeY16, PIXEL_FORMAT_Y16},
      {kMediaSubTypeZ16, PIXEL_FORMAT_Y16},
      {kMediaSubTypeINVZ, PIXEL_FORMAT_Y16},
  };

  for (const auto& kFormat : kFormatMap) {
    if (kFormat.guid == guid) {
      *format = kFormat.format;
      return true;
    }
  }

  return false;
}

VideoCaptureDeviceMFWin::VideoCaptureDeviceMFWin(
    const VideoCaptureDeviceDescriptor& device_descriptor)
    : descriptor_(device_descriptor), capture_(0) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

VideoCaptureDeviceMFWin::~VideoCaptureDeviceMFWin() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

bool VideoCaptureDeviceMFWin::Init(const ComPtr<IMFMediaSource>& source) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!engine_.Get());

  base::AutoLock lock(lock_);

  HRESULT hr = S_OK;
  ComPtr<IMFAttributes> attributes;
  ComPtr<IMFCaptureEngineClassFactory> captureEngineClassFactory;
  MFCreateAttributes(attributes.GetAddressOf(), 1);
  DCHECK(attributes.Get());

  hr = CoCreateInstance(CLSID_MFCaptureEngineClassFactory, NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_PPV_ARGS(captureEngineClassFactory.GetAddressOf()));
  if (FAILED(hr)) {
    LogError(FROM_HERE, hr);
    return false;
  }
  hr = captureEngineClassFactory->CreateInstance(
      CLSID_MFCaptureEngine, IID_PPV_ARGS(engine_.GetAddressOf()));
  if (FAILED(hr)) {
    LogError(FROM_HERE, hr);
    return false;
  }

  callback_ = new MFVideoCallback(this);
  if (FAILED(hr)) {
    LogError(FROM_HERE, hr);
    return false;
  }

  hr = engine_->Initialize(callback_.get(), attributes.Get(), NULL,
                           source.Get());
  if (FAILED(hr)) {
    LogError(FROM_HERE, hr);
    return false;
  }

  return true;
}

void VideoCaptureDeviceMFWin::AllocateAndStart(
    const VideoCaptureParams& params,
    std::unique_ptr<VideoCaptureDevice::Client> client) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  base::AutoLock lock(lock_);

  client_ = std::move(client);
  DCHECK_EQ(capture_, false);

  CapabilityList capabilities;
  Location from_here = FROM_HERE;
  HRESULT hr = S_OK;
  if (!engine_.Get()) {
    hr = E_FAIL;
  }

  ComPtr<IMFCaptureSource> source;
  if (SUCCEEDED(hr)) {
    hr = engine_->GetSource(source.GetAddressOf());
    from_here = FROM_HERE;
  }
  if (SUCCEEDED(hr)) {
    hr = FillCapabilities(kPreferredVideoPreviewStream, source.Get(),
                          &capabilities);
    from_here = FROM_HERE;
  }

  if (SUCCEEDED(hr)) {
    const CapabilityWin found_capability =
        GetBestMatchedCapability(params.requested_format, capabilities);
    ComPtr<IMFMediaType> type;
    hr = GetAvailableDeviceMediaType(source.Get(), kPreferredVideoPreviewStream,
                                     found_capability.stream_index,
                                     type.GetAddressOf());
    from_here = FROM_HERE;

    if (SUCCEEDED(hr)) {
      hr = source.Get()->SetCurrentDeviceMediaType(kPreferredVideoPreviewStream,
                                                   type.Get());
      from_here = FROM_HERE;
    }

    ComPtr<IMFCaptureSink> sink;
    if (SUCCEEDED(hr)) {
      hr = engine_->GetSink(MF_CAPTURE_ENGINE_SINK_TYPE_PREVIEW,
                            sink.GetAddressOf());
      from_here = FROM_HERE;
    }

    ComPtr<IMFCapturePreviewSink> preview_sink;
    if (SUCCEEDED(hr)) {
      hr =
          sink.Get()->QueryInterface(IID_PPV_ARGS(preview_sink.GetAddressOf()));
      from_here = FROM_HERE;
    }
    if (SUCCEEDED(hr)) {
      hr = preview_sink->RemoveAllStreams();
      from_here = FROM_HERE;
    }

    DWORD dwSinkStreamIndex = NULL;
    if (SUCCEEDED(hr)) {
      hr = preview_sink->AddStream(kPreferredVideoPreviewStream, type.Get(),
                                   NULL, &dwSinkStreamIndex);
      from_here = FROM_HERE;
    }
    if (SUCCEEDED(hr)) {
      hr = preview_sink->SetSampleCallback(dwSinkStreamIndex, callback_.get());
      from_here = FROM_HERE;
    }
    if (SUCCEEDED(hr)) {
      hr = engine_->StartPreview();
      from_here = FROM_HERE;
    }

    if (SUCCEEDED(hr)) {
      capture_video_format_ = found_capability.supported_format;
      client_->OnStarted();
      capture_ = true;
    }
  }

  if (FAILED(hr)) {
    OnError(from_here, hr);
  }
}

void VideoCaptureDeviceMFWin::StopAndDeAllocate() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::AutoLock lock(lock_);
  if (capture_) {
    capture_ = false;
    if (engine_.Get()) {
      engine_->StopPreview();
    }
  }

  client_.reset();
}

void VideoCaptureDeviceMFWin::TakePhoto(TakePhotoCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (capture_) {
    HRESULT hr = S_OK;
    Location from_here = FROM_HERE;

    ComPtr<IMFCaptureSource> source;
    hr = engine_->GetSource(source.GetAddressOf());
    from_here = FROM_HERE;

    ComPtr<IMFMediaType> currentMediaType;
    if (SUCCEEDED(hr)) {
      hr = source.Get()->GetCurrentDeviceMediaType(
          kPreferredPhotoStream, currentMediaType.GetAddressOf());
      from_here = FROM_HERE;
    }

    ComPtr<IMFMediaType> photoMediaType;
    if (SUCCEEDED(hr)) {
      hr = MFCreateMediaType(photoMediaType.GetAddressOf());
      from_here = FROM_HERE;
    }

    if (SUCCEEDED(hr)) {
      hr = ConvertToPhotoMediaType(currentMediaType.Get(), photoMediaType.Get())
               ? S_OK
               : E_FAIL;
      from_here = FROM_HERE;
    }

    if (SUCCEEDED(hr)) {
      hr = source.Get()->SetCurrentDeviceMediaType(kPreferredPhotoStream,
                                                   photoMediaType.Get());
      from_here = FROM_HERE;
    }

    VideoCaptureFormat format;
    if (SUCCEEDED(hr)) {
      hr = FillFormat(photoMediaType.Get(), &format) ? S_OK : E_FAIL;
      from_here = FROM_HERE;
    }
    ComPtr<IMFCaptureSink> sink;
    if (SUCCEEDED(hr)) {
      hr = engine_->GetSink(MF_CAPTURE_ENGINE_SINK_TYPE_PHOTO,
                            sink.GetAddressOf());
      from_here = FROM_HERE;
    }
    ComPtr<IMFCapturePhotoSink> photoSink;
    if (SUCCEEDED(hr)) {
      hr = sink.Get()->QueryInterface(IID_PPV_ARGS(photoSink.GetAddressOf()));
      from_here = FROM_HERE;
    }
    if (SUCCEEDED(hr)) {
      hr = photoSink->RemoveAllStreams();
      from_here = FROM_HERE;
    }
    DWORD dwSinkStreamIndex;
    if (SUCCEEDED(hr)) {
      hr = photoSink.Get()->AddStream(kPreferredPhotoStream,
                                      photoMediaType.Get(), NULL,
                                      &dwSinkStreamIndex);
      from_here = FROM_HERE;
    }
    if (SUCCEEDED(hr)) {
      hr = photoSink.Get()->SetSampleCallback(
          new MFPhotoCallback(std::move(callback), std::move(format)));
      from_here = FROM_HERE;
    }
    if (SUCCEEDED(hr)) {
      hr = engine_.Get()->TakePhoto();
      from_here = FROM_HERE;
    }

    if (FAILED(hr)) {
      LogError(from_here, hr);
    }
  }
}

void VideoCaptureDeviceMFWin::GetPhotoState(GetPhotoStateCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (capture_) {
    HRESULT hr = S_OK;
    Location from_here = FROM_HERE;

    ComPtr<IMFCaptureSource> source;
    if (SUCCEEDED(hr)) {
      hr = engine_->GetSource(source.GetAddressOf());
      from_here = FROM_HERE;
    }
    CapabilityList capabilities;
    if (SUCCEEDED(hr)) {
      hr = FillCapabilities(kPreferredPhotoStream, source.Get(), &capabilities);
      from_here = FROM_HERE;
    }
    ComPtr<IMFMediaType> currentMediaType;
    if (SUCCEEDED(hr)) {
      hr = source.Get()->GetCurrentDeviceMediaType(
          kPreferredPhotoStream, currentMediaType.GetAddressOf());
      from_here = FROM_HERE;
    }

    if (SUCCEEDED(hr)) {
      auto photo_capabilities = mojom::PhotoState::New();
      gfx::Size currentSize;
      GetFrameSize(currentMediaType.Get(), &currentSize);

      int minHeight = currentSize.height();
      int currentHeight = currentSize.height();
      int maxHeight = currentSize.height();
      int minWidth = currentSize.width();
      int currentWidth = currentSize.width();
      int maxWidth = currentSize.width();
      for (const CapabilityWin& capability : capabilities) {
        int height = capability.supported_format.frame_size.height();
        if (height > maxHeight) {
          maxHeight = height;
        }
        if (height < minHeight) {
          minHeight = height;
        }
        int width = capability.supported_format.frame_size.width();
        if (width > maxWidth) {
          maxWidth = width;
        }
        if (width < minWidth) {
          minWidth = width;
        }
      }

      photo_capabilities->height =
          mojom::Range::New(maxHeight, minHeight, currentHeight, 1);
      photo_capabilities->width =
          mojom::Range::New(maxWidth, minWidth, currentWidth, 1);

      photo_capabilities->exposure_compensation = mojom::Range::New();
      photo_capabilities->color_temperature = mojom::Range::New();
      photo_capabilities->iso = mojom::Range::New();
      photo_capabilities->brightness = mojom::Range::New();
      photo_capabilities->contrast = mojom::Range::New();
      photo_capabilities->saturation = mojom::Range::New();
      photo_capabilities->sharpness = mojom::Range::New();
      photo_capabilities->zoom = mojom::Range::New();
      photo_capabilities->red_eye_reduction = mojom::RedEyeReduction::NEVER;

      photo_capabilities->torch = false;
      std::move(callback).Run(std::move(photo_capabilities));
    }

    if (FAILED(hr)) {
      LogError(from_here, hr);
    }
  }
}

void VideoCaptureDeviceMFWin::SetPhotoOptions(
    mojom::PhotoSettingsPtr settings,
    SetPhotoOptionsCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (capture_) {
    HRESULT hr = S_OK;
    Location from_here = FROM_HERE;
    ComPtr<IMFCaptureSource> source;
    hr = engine_->GetSource(source.GetAddressOf());
    from_here = FROM_HERE;

    if (SUCCEEDED(hr) && (settings->has_height || settings->has_width)) {
      CapabilityList capabilities;
      hr = FillCapabilities(kPreferredPhotoStream, source.Get(), &capabilities);
      from_here = FROM_HERE;

      ComPtr<IMFMediaType> currentMediaType;
      if (SUCCEEDED(hr)) {
        hr = source.Get()->GetCurrentDeviceMediaType(
            kPreferredPhotoStream, currentMediaType.GetAddressOf());
        from_here = FROM_HERE;
      }

      if (SUCCEEDED(hr)) {
        gfx::Size currentSize;
        GetFrameSize(currentMediaType.Get(), &currentSize);
        int requestedHeight = currentSize.height();
        if (settings->has_height) {
          requestedHeight = settings->height;
        }
        int requestedWidth = currentSize.width();
        if (settings->has_width) {
          requestedWidth = settings->width;
        }
        const CapabilityWin* best_match = &(*capabilities.begin());
        for (const CapabilityWin& capability : capabilities) {
          int height = capability.supported_format.frame_size.height();
          int width = capability.supported_format.frame_size.width();
          if (std::abs(height - requestedHeight) <=
                  std::abs(height -
                           best_match->supported_format.frame_size.height()) &&
              std::abs(width - requestedWidth) <=
                  std::abs(width -
                           best_match->supported_format.frame_size.width())) {
            best_match = &capability;
          }
        }

        ComPtr<IMFMediaType> type;
        hr = GetAvailableDeviceMediaType(source.Get(), kPreferredPhotoStream,
                                         best_match->stream_index,
                                         type.GetAddressOf());
        from_here = FROM_HERE;
        if (SUCCEEDED(hr)) {
          source.Get()->SetCurrentDeviceMediaType(kPreferredPhotoStream,
                                                  type.Get());
        }
      }
    }

    if (SUCCEEDED(hr)) {
      std::move(callback).Run(true);
    } else {
      LogError(from_here, hr);
    }
  }
}

void VideoCaptureDeviceMFWin::OnIncomingCapturedData(
    const uint8_t* data,
    int length,
    int rotation,
    base::TimeTicks reference_time,
    base::TimeDelta timestamp) {
  base::AutoLock lock(lock_);
  if (data && client_.get()) {
    client_->OnIncomingCapturedData(data, length, capture_video_format_,
                                    rotation, reference_time, timestamp);
  }
}

void VideoCaptureDeviceMFWin::OnEvent(IMFMediaEvent* mediaEvent) {
  HRESULT hr = S_OK;
  GUID eventType;
  hr = mediaEvent->GetExtendedType(&eventType);

  if (SUCCEEDED(hr)) {
    if (eventType == MF_CAPTURE_ENGINE_ERROR) {
      mediaEvent->GetStatus(&hr);
    }
  }

  if (FAILED(hr)) {
    OnError(FROM_HERE, hr);
  }
}

void VideoCaptureDeviceMFWin::OnError(const Location& from_here, HRESULT hr) {
  if (client_.get()) {
    client_->OnError(
        from_here,
        base::StringPrintf("VideoCaptureDeviceMFWin: %s",
                           logging::SystemErrorCodeToString(hr).c_str()));
  }
}

}  // namespace media