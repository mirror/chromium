// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/shape_detection/face_detection_impl_win.h"

#include <cfgmgr32.h>
#include <comdef.h>
#include <devpkey.h>
#include <initguid.h>  // Required by <devpkey.h>
#include <iomanip>
#pragma warning(disable : 4467)
#include <robuffer.h>
#pragma warning(default : 4467)

#include <windows.graphics.imaging.h>
#include <wrl/event.h>

#include "base/scoped_generic.h"
#include "base/win/windows_version.h"
#include "media/base/scoped_callback_runner.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/shape_detection/face_detection_provider_impl.h"

namespace shape_detection {

namespace WRL = Microsoft::WRL;

using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::Graphics::Imaging;
using namespace ABI::Windows::Media::FaceAnalysis;
using namespace ABI::Windows::Storage::Streams;
using namespace ABI::Windows::UI::Xaml::Media::Imaging;

using base::win::ScopedComPtr;

namespace {

// TODO(Junwei): Merge com base functions with media/midi/midi_manager_winrt.cc.
// Helpers for printing HRESULTs.
struct PrintHr {
  PrintHr(HRESULT hr) : hr(hr) {}
  HRESULT hr;
};

std::ostream& operator<<(std::ostream& os, const PrintHr& phr) {
  std::ios_base::fmtflags ff = os.flags();
  os << _com_error(phr.hr).ErrorMessage() << " (0x" << std::hex
     << std::uppercase << std::setfill('0') << std::setw(8) << phr.hr << ")";
  os.flags(ff);
  return os;
}

// Provides access to functions in combase.dll which may not be available on
// Windows 7. Loads functions dynamically at runtime to prevent library
// dependencies.
class CombaseFunctions {
 public:
  CombaseFunctions() = default;

  ~CombaseFunctions() {
    if (combase_dll_)
      ::FreeLibrary(combase_dll_);
  }

  bool LoadFunctions() {
    combase_dll_ = ::LoadLibrary(L"combase.dll");
    if (!combase_dll_)
      return false;

    get_factory_func_ = reinterpret_cast<decltype(&::RoGetActivationFactory)>(
        ::GetProcAddress(combase_dll_, "RoGetActivationFactory"));
    if (!get_factory_func_)
      return false;

    create_string_func_ = reinterpret_cast<decltype(&::WindowsCreateString)>(
        ::GetProcAddress(combase_dll_, "WindowsCreateString"));
    if (!create_string_func_)
      return false;

    delete_string_func_ = reinterpret_cast<decltype(&::WindowsDeleteString)>(
        ::GetProcAddress(combase_dll_, "WindowsDeleteString"));
    if (!delete_string_func_)
      return false;

    get_string_raw_buffer_func_ =
        reinterpret_cast<decltype(&::WindowsGetStringRawBuffer)>(
            ::GetProcAddress(combase_dll_, "WindowsGetStringRawBuffer"));
    if (!get_string_raw_buffer_func_)
      return false;

    return true;
  }

  HRESULT RoGetActivationFactory(HSTRING class_id,
                                 const IID& iid,
                                 void** out_factory) {
    DCHECK(get_factory_func_);
    return get_factory_func_(class_id, iid, out_factory);
  }

  HRESULT WindowsCreateString(const base::char16* src,
                              uint32_t len,
                              HSTRING* out_hstr) {
    DCHECK(create_string_func_);
    return create_string_func_(src, len, out_hstr);
  }

  HRESULT WindowsDeleteString(HSTRING hstr) {
    DCHECK(delete_string_func_);
    return delete_string_func_(hstr);
  }

  const base::char16* WindowsGetStringRawBuffer(HSTRING hstr,
                                                uint32_t* out_len) {
    DCHECK(get_string_raw_buffer_func_);
    return get_string_raw_buffer_func_(hstr, out_len);
  }

 private:
  HMODULE combase_dll_ = nullptr;

  decltype(&::RoGetActivationFactory) get_factory_func_ = nullptr;
  decltype(&::WindowsCreateString) create_string_func_ = nullptr;
  decltype(&::WindowsDeleteString) delete_string_func_ = nullptr;
  decltype(&::WindowsGetStringRawBuffer) get_string_raw_buffer_func_ = nullptr;
};

CombaseFunctions* GetCombaseFunctions() {
  static CombaseFunctions* functions = new CombaseFunctions();
  return functions;
}

// Scoped HSTRING class to maintain lifetime of HSTRINGs allocated with
// WindowsCreateString().
class ScopedHStringTraits {
 public:
  static HSTRING InvalidValue() { return nullptr; }

  static void Free(HSTRING hstr) {
    GetCombaseFunctions()->WindowsDeleteString(hstr);
  }
};

class ScopedHString : public base::ScopedGeneric<HSTRING, ScopedHStringTraits> {
 public:
  explicit ScopedHString(const base::char16* str) : ScopedGeneric(nullptr) {
    HSTRING hstr;
    HRESULT hr = GetCombaseFunctions()->WindowsCreateString(
        str, static_cast<uint32_t>(wcslen(str)), &hstr);
    if (FAILED(hr))
      VLOG(1) << "WindowsCreateString failed: " << PrintHr(hr);
    else
      reset(hstr);
  }
};

template <typename InterfaceType, base::char16 const* runtime_class_id>
ScopedComPtr<InterfaceType> WrlStaticsFactory() {
  ScopedComPtr<InterfaceType> com_ptr;

  ScopedHString class_id_hstring(runtime_class_id);
  if (!class_id_hstring.is_valid()) {
    com_ptr = nullptr;
    return com_ptr;
  }

  HRESULT hr = GetCombaseFunctions()->RoGetActivationFactory(
      class_id_hstring.get(), IID_PPV_ARGS(&com_ptr));
  if (FAILED(hr)) {
    VLOG(1) << "RoGetActivationFactory failed: " << PrintHr(hr);
    com_ptr = nullptr;
  }

  return com_ptr;
}

HRESULT GetPointerToBufferData(IBuffer* buffer, uint8_t** out) {
  ScopedComPtr<Windows::Storage::Streams::IBufferByteAccess> buffer_byte_access;

  HRESULT hr = buffer->QueryInterface(IID_PPV_ARGS(&buffer_byte_access));
  if (FAILED(hr)) {
    VLOG(1) << "QueryInterface failed: " << PrintHr(hr);
    return hr;
  }

  // Lifetime of the pointing buffer is controlled by the buffer object.
  hr = buffer_byte_access->Buffer(out);
  if (FAILED(hr)) {
    VLOG(1) << "Buffer failed: " << PrintHr(hr);
    return hr;
  }

  return S_OK;
}

}  // anonymous namespace

void FaceDetectionProviderImpl::CreateFaceDetection(
    shape_detection::mojom::FaceDetectionRequest request,
    shape_detection::mojom::FaceDetectorOptionsPtr options) {
  // FaceDetector class is only available in Win 10 onwards (v10.0.10240.0)
  if (base::win::GetVersion() < base::win::VERSION_WIN10) {
    DVLOG(1) << "FaceDetector not supported";
    return;
  }
  mojo::MakeStrongBinding(
      base::MakeUnique<FaceDetectionImplWin>(std::move(options)),
      std::move(request));
}

FaceDetectionImplWin::FaceDetectionImplWin(
    shape_detection::mojom::FaceDetectorOptionsPtr options)
    : async_create_detector_ops_(this), async_detected_face_ops_(this) {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!GetCombaseFunctions()->LoadFunctions()) {
    DLOG(ERROR) << "Failed loading functions from combase.dll: "
                << PrintHr(HRESULT_FROM_WIN32(GetLastError()));
    return;
  }

  detector_factory_ =
      WrlStaticsFactory<IFaceDetectorStatics,
                        RuntimeClass_Windows_Media_FaceAnalysis_FaceDetector>();

  boolean is_supported = false;
  detector_factory_->get_IsSupported(&is_supported);
  DCHECK(is_supported);

#if DCHECK_IS_ON()
  boolean is_gray8_supported = false;
  detector_factory_->IsBitmapPixelFormatSupported(BitmapPixelFormat_Gray8,
                                                  &is_gray8_supported);
  DCHECK(is_gray8_supported) << " Gray8 pixel format should be supported";

  boolean is_nv12_supported = false;
  detector_factory_->IsBitmapPixelFormatSupported(BitmapPixelFormat_Nv12,
                                                  &is_nv12_supported);
  DCHECK(is_nv12_supported) << " NV12 pixel format should be supported";
#endif  // DCHECK_IS_ON()
}

FaceDetectionImplWin::~FaceDetectionImplWin() {}

void FaceDetectionImplWin::Detect(const SkBitmap& bitmap,
                                  DetectCallback callback) {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());

  // TODO(Junwei): Vectors hold |callback| and |bitmap| for detecting seriality.
  // |detecting_bitmap_| will ensure serial detecting
  detect_callback_ = std::move(callback);
  bitmap_ = bitmap;

  if (detector_) {
    asyncDetectSoftwareBitmap();
  } else {
    asyncCreateFaceDetector();
  }
}

void FaceDetectionImplWin::asyncCreateFaceDetector() {
  IAsyncOperation<FaceDetector*>* async_op;
  HRESULT hr = detector_factory_->CreateAsync(&async_op);
  if (FAILED(hr)) {
    DLOG(ERROR) << "CreateAsync() failed: " << PrintHr(hr);
    return;
  }

  async_create_detector_ops_.asyncOperate(async_op);
}

class FaceDetectionImplWin::AsyncCreateFaceDetector final
    : public AsyncOperationTemplate<FaceDetector, FaceDetectionImplWin> {
 public:
  AsyncCreateFaceDetector(FaceDetectionImplWin* face_detection_impl)
      : AsyncOperationTemplate(face_detection_impl), weak_factory_(this) {}

 private:
  void asyncCallback(IAsyncOperation<FaceDetector*>* async_op) final {
    detection_impl_->OnFaceDetectorConstructed(async_op);
  }
  base::WeakPtr<AsyncOperationTemplate> GetWeakPtrFromFactory() final {
    return weak_factory_.GetWeakPtr();
  }
  base::WeakPtrFactory<AsyncCreateFaceDetector> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AsyncCreateFaceDetector);
};

void FaceDetectionImplWin::OnFaceDetectorConstructed(
    IAsyncOperation<FaceDetector*>* async_op) {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());

  HRESULT hr = async_op->GetResults(detector_.GetAddressOf());
  if (FAILED(hr)) {
    DLOG(ERROR) << "GetResults failed: " << PrintHr(hr);
    return;
  }

  if (!bitmap_factory_) {
    bitmap_factory_ = WrlStaticsFactory<
        ISoftwareBitmapStatics,
        RuntimeClass_Windows_Graphics_Imaging_SoftwareBitmap>();
  }
  DCHECK(bitmap_factory_);

  asyncDetectSoftwareBitmap();
}

void FaceDetectionImplWin::asyncDetectSoftwareBitmap() {
  ScopedComPtr<ISoftwareBitmap> win_bitmap = convertToSoftwareBitmap(bitmap_);
  if (!win_bitmap)
    return;

  if (!detector_ || !bitmap_factory_) {
    DVLOG(1) << "Detector and/or factories not ready";
    return;
  }

  IAsyncOperation<IVector<DetectedFace*>*>* async_op;
  HRESULT hr = detector_->DetectFacesAsync(win_bitmap.Get(), &async_op);
  if (FAILED(hr)) {
    DLOG(ERROR) << "DetectFacesAsync() failed: " << PrintHr(hr);
    return;
  }

  async_detected_face_ops_.asyncOperate(async_op);
}

class AsyncDetectFace final
    : public AsyncOperationTemplate<IVector<DetectedFace*>,
                                    FaceDetectionImplWin> {
 public:
  AsyncDetectFace(FaceDetectionImplWin* face_detection_impl)
      : AsyncOperationTemplate(face_detection_impl), weak_factory_(this) {}

 private:
  void asyncCallback(IAsyncOperation<IVector<DetectedFace*>*>* async_op) final {
    detection_impl_->OnFacesDetected(async_op);
  }
  base::WeakPtr<AsyncOperationTemplate> GetWeakPtrFromFactory() final {
    return weak_factory_.GetWeakPtr();
  }
  base::WeakPtrFactory<AsyncDetectFace> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AsyncDetectFace);
};

void FaceDetectionImplWin::OnFacesDetected(
    IAsyncOperation<IVector<DetectedFace*>*>* async_op) {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());

  HRESULT hr = async_op->GetResults(detected_face_.GetAddressOf());
  if (FAILED(hr)) {
    DLOG(ERROR) << "GetResults failed: " << PrintHr(hr);
    return;
  }

  DetectCallback scoped_callback =
      media::ScopedCallbackRunner(std::move(detect_callback_),
                                  std::vector<mojom::FaceDetectionResultPtr>());
  std::vector<mojom::FaceDetectionResultPtr> results;

  uint32_t count;
  hr = detected_face_->get_Size(&count);
  if (FAILED(hr)) {
    DLOG(ERROR) << "get_Size failed: " << PrintHr(hr);
    return;
  }
  for (uint32_t i = 0; i < count; i++) {
    ScopedComPtr<IDetectedFace> face;
    hr = detected_face_->GetAt(i, &face);

    BitmapBounds bounds;
    hr = face->get_FaceBox(&bounds);
    if (FAILED(hr)) {
      DLOG(ERROR) << "get_FaceBox failed: " << PrintHr(hr);
      return;
    }
    gfx::RectF boundingbox(bounds.X, bounds.Y, bounds.Width, bounds.Height);

    auto result = shape_detection::mojom::FaceDetectionResult::New();
    result->bounding_box = std::move(boundingbox);
    results.push_back(std::move(result));
  }

  std::move(scoped_callback).Run(std::move(results));
}

ScopedComPtr<ISoftwareBitmap> FaceDetectionImplWin::convertToSoftwareBitmap(
    const SkBitmap& bitmap) {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());

  auto buffer_factory =
      WrlStaticsFactory<IBufferFactory,
                        RuntimeClass_Windows_Storage_Streams_Buffer>();
  if (!buffer_factory)
    return ScopedComPtr<ISoftwareBitmap>();

  ScopedComPtr<IBuffer> buffer;
  const UINT32 length = static_cast<UINT32>(bitmap.getSize());
  HRESULT hr = buffer_factory->Create(length, buffer.GetAddressOf());
  if (FAILED(hr)) {
    VLOG(1) << "Create failed: " << PrintHr(hr);
    return ScopedComPtr<ISoftwareBitmap>();
  }

  hr = buffer->put_Length(length);
  if (FAILED(hr)) {
    VLOG(1) << "put_Length failed: " << PrintHr(hr);
    return ScopedComPtr<ISoftwareBitmap>();
  }

  uint8_t* p_buffer_data = nullptr;
  hr = GetPointerToBufferData(buffer.Get(), &p_buffer_data);
  if (FAILED(hr))
    return ScopedComPtr<ISoftwareBitmap>();

  memcpy(p_buffer_data, static_cast<uint8_t*>(bitmap.getPixels()), length);

  // Allocate an internal rgba8 SoftwareBitmap.
  const BitmapPixelFormat pixelFormat =
      (kN32_SkColorType == kRGBA_8888_SkColorType) ? BitmapPixelFormat_Rgba8
                                                   : BitmapPixelFormat_Bgra8;

  // Allocate an internal kN32_SkColorType SoftwareBitmap.
  ScopedComPtr<ISoftwareBitmap> win_bitmap;
  // // Finally create ISoftwareBitmap |win_bitmap| and copy IBuffer |buffer|
  // into it.
  hr = bitmap_factory_->CreateCopyFromBuffer(buffer.Get(), pixelFormat,
                                             bitmap.width(), bitmap.height(),
                                             win_bitmap.GetAddressOf());
  if (FAILED(hr)) {
    DLOG(ERROR) << "Create() failed: " << PrintHr(hr) << ", "
                << PrintHr(HRESULT_FROM_WIN32(GetLastError()));
    return ScopedComPtr<ISoftwareBitmap>();
  }

  hr = bitmap_factory_->Convert(win_bitmap.Get(), BitmapPixelFormat_Gray8,
                                win_bitmap.GetAddressOf());
  if (FAILED(hr)) {
    DLOG(ERROR) << "CopyFromBuffer() failed: " << PrintHr(hr);
    return ScopedComPtr<ISoftwareBitmap>();
  }

  return win_bitmap;
}

}  // namespace shape_detection