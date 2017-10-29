// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media_capture_from_element/canvas_capture_handler.h"

#include <utility>

#include "base/base64.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/rand_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/viz/common/gl_helper.h"
#include "content/public/renderer/render_thread.h"
#include "content/renderer/media/media_stream_video_capturer_source.h"
#include "content/renderer/media/media_stream_video_source.h"
#include "content/renderer/media/media_stream_video_track.h"
#include "content/renderer/media/webrtc_uma_histograms.h"
#include "content/renderer/render_thread_impl.h"
#include "content/renderer/webgraphicscontext3d_provider_impl.h"
#include "media/base/limits.h"
#include "services/ui/public/cpp/gpu/context_provider_command_buffer.h"
#include "skia/ext/texture_handle.h"
#include "third_party/WebKit/public/platform/WebGraphicsContext3DProvider.h"
#include "third_party/WebKit/public/platform/WebMediaStreamSource.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/libyuv/include/libyuv.h"
#include "third_party/skia/include/core/SkImage.h"

using media::VideoFrame;

namespace {
const size_t kArgbBytesPerPixel = 4;
}  // namespace

namespace content {

// Implementation VideoCapturerSource that is owned by
// MediaStreamVideoCapturerSource and delegates the Start/Stop calls to
// CanvasCaptureHandler.
// This class is single threaded and pinned to main render thread.
class VideoCapturerSource : public media::VideoCapturerSource {
 public:
  VideoCapturerSource(base::WeakPtr<CanvasCaptureHandler> canvas_handler,
                      const blink::WebSize& size,
                      double frame_rate)
      : size_(size),
        frame_rate_(static_cast<float>(
            std::min(static_cast<double>(media::limits::kMaxFramesPerSecond),
                     frame_rate))),
        canvas_handler_(canvas_handler) {
    DCHECK_LE(0, frame_rate_);
  }

 protected:
  media::VideoCaptureFormats GetPreferredFormats() override {
    DCHECK(main_render_thread_checker_.CalledOnValidThread());
    media::VideoCaptureFormats formats;
    formats.push_back(
        media::VideoCaptureFormat(gfx::Size(size_.width, size_.height),
                                  frame_rate_, media::PIXEL_FORMAT_I420));
    formats.push_back(
        media::VideoCaptureFormat(gfx::Size(size_.width, size_.height),
                                  frame_rate_, media::PIXEL_FORMAT_YV12A));
    return formats;
  }
  void StartCapture(const media::VideoCaptureParams& params,
                    const VideoCaptureDeliverFrameCB& frame_callback,
                    const RunningCallback& running_callback) override {
    DCHECK(main_render_thread_checker_.CalledOnValidThread());
    canvas_handler_->StartVideoCapture(params, frame_callback,
                                       running_callback);
  }
  void RequestRefreshFrame() override {
    DCHECK(main_render_thread_checker_.CalledOnValidThread());
    canvas_handler_->RequestRefreshFrame();
  }
  void StopCapture() override {
    DCHECK(main_render_thread_checker_.CalledOnValidThread());
    if (canvas_handler_.get())
      canvas_handler_->StopVideoCapture();
  }

 private:
  const blink::WebSize size_;
  const float frame_rate_;
  // Bound to Main Render thread.
  base::ThreadChecker main_render_thread_checker_;
  // CanvasCaptureHandler is owned by CanvasDrawListener in blink and might be
  // destroyed before StopCapture() call.
  base::WeakPtr<CanvasCaptureHandler> canvas_handler_;
};

class CanvasCaptureHandler::CanvasCaptureHandlerDelegate {
 public:
  explicit CanvasCaptureHandlerDelegate(
      media::VideoCapturerSource::VideoCaptureDeliverFrameCB new_frame_callback)
      : new_frame_callback_(new_frame_callback), weak_ptr_factory_(this) {
    io_thread_checker_.DetachFromThread();
  }
  ~CanvasCaptureHandlerDelegate() {
    DCHECK(io_thread_checker_.CalledOnValidThread());
  }

  void SendNewFrameOnIOThread(const scoped_refptr<VideoFrame>& video_frame,
                              const base::TimeTicks& current_time) {
    DCHECK(io_thread_checker_.CalledOnValidThread());
    new_frame_callback_.Run(video_frame, current_time);
  }

  base::WeakPtr<CanvasCaptureHandlerDelegate> GetWeakPtrForIOThread() {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  const media::VideoCapturerSource::VideoCaptureDeliverFrameCB
      new_frame_callback_;
  // Bound to IO thread.
  base::ThreadChecker io_thread_checker_;
  base::WeakPtrFactory<CanvasCaptureHandlerDelegate> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CanvasCaptureHandlerDelegate);
};

CanvasCaptureHandler::CanvasCaptureHandler(
    const blink::WebSize& size,
    double frame_rate,
    const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner,
    blink::WebMediaStreamTrack* track)
    : ask_for_new_frame_(false),
      io_task_runner_(io_task_runner),
      weak_ptr_factory_(this) {
  std::unique_ptr<media::VideoCapturerSource> video_source(
      new VideoCapturerSource(weak_ptr_factory_.GetWeakPtr(), size,
                              frame_rate));
  AddVideoCapturerSourceToVideoTrack(std::move(video_source), track);
}

CanvasCaptureHandler::~CanvasCaptureHandler() {
  DVLOG(3) << __func__;
  DCHECK(main_render_thread_checker_.CalledOnValidThread());
  io_task_runner_->DeleteSoon(FROM_HERE, delegate_.release());
}

// static
std::unique_ptr<CanvasCaptureHandler>
CanvasCaptureHandler::CreateCanvasCaptureHandler(
    const blink::WebSize& size,
    double frame_rate,
    const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner,
    blink::WebMediaStreamTrack* track) {
  // Save histogram data so we can see how much CanvasCapture is used.
  // The histogram counts the number of calls to the JS API.
  UpdateWebRTCMethodCount(WEBKIT_CANVAS_CAPTURE_STREAM);

  return std::unique_ptr<CanvasCaptureHandler>(
      new CanvasCaptureHandler(size, frame_rate, io_task_runner, track));
}

bool CanvasCaptureHandler::NeedsNewFrame() const {
  DCHECK(main_render_thread_checker_.CalledOnValidThread());
  return ask_for_new_frame_;
}

void CanvasCaptureHandler::SendNewFrame(
    sk_sp<SkImage> image,
    blink::WebGraphicsContext3DProvider* context_provider) {
  DVLOG(4) << __func__;
  DCHECK(main_render_thread_checker_.CalledOnValidThread());
  DCHECK(image);
  TRACE_EVENT0("webrtc", "CanvasCaptureHandler::DeliverNewFrame");

  // Initially try accessing pixels directly if they are in memory.
  SkPixmap pixmap;
  if (image->peekPixels(&pixmap) &&
      (pixmap.colorType() == kRGBA_8888_SkColorType ||
       pixmap.colorType() == kBGRA_8888_SkColorType) &&
      pixmap.alphaType() == kUnpremul_SkAlphaType) {
    SendAsYUVFrame(image->isOpaque(), false,
                   static_cast<const uint8*>(pixmap.addr(0, 0)),
                   gfx::Size(pixmap.width(), pixmap.height()),
                   pixmap.rowBytes(), pixmap.colorType());
    return;
  }

  const gfx::Size image_size(image->width(), image->height());
  // Allocate or reuse a temp ARGB frame to read texture info into.
  // TODO(emircan): Currently only YUVPlanar frames are allowed in
  // VideoFramePool. Replace this with allocation of an actual ARGB frame when
  // http://crbug.com/555909 is resolved.
  const gfx::Size argb_adjusted_size(kArgbBytesPerPixel * image_size.width(),
                                     image_size.height());
  scoped_refptr<VideoFrame> temp_argb_frame = frame_pool_.CreateFrame(
      media::PIXEL_FORMAT_I420, argb_adjusted_size,
      gfx::Rect(argb_adjusted_size), argb_adjusted_size, base::TimeDelta());
  if (!temp_argb_frame) {
    DLOG(ERROR) << "Couldn't allocate temp memory";
    return;
  }

  // Try async reading SkImage if it is texture backed.
  if (image->isTextureBacked()) {
    if (context_provider) {
      if (!gl_helper_) {
        content::WebGraphicsContext3DProviderImpl* context_provider_impl =
            static_cast<content::WebGraphicsContext3DProviderImpl*>(
                context_provider);
        DCHECK(context_provider_impl);
        context_provider_impl->SetLostContextCallback(
            base::Bind(&CanvasCaptureHandler::OnContextLost,
                       weak_ptr_factory_.GetWeakPtr()));
        DCHECK(context_provider_impl->context_provider());
        gl_helper_.reset(new viz::GLHelper(
            context_provider_impl->ContextGL(),
            context_provider_impl->context_provider()->ContextSupport()));
      }
      GrSurfaceOrigin surface_origin = kTopLeft_GrSurfaceOrigin;
      const GrGLTextureInfo* texture_info =
          skia::GrBackendObjectToGrGLTextureInfo(
              image->getTextureHandle(true, &surface_origin));
      DCHECK(texture_info);
      gl_helper_->ReadbackTextureAsync(
          texture_info->fID, image_size,
          temp_argb_frame->visible_data(VideoFrame::kYPlane), kN32_SkColorType,
          base::Bind(&CanvasCaptureHandler::ReadPixelsAsync,
                     weak_ptr_factory_.GetWeakPtr(), image, temp_argb_frame,
                     surface_origin != kTopLeft_GrSurfaceOrigin));
      return;
    }
    // |context_provider| being nullptr indicates a context loss.
    gl_helper_.reset();
  }

  // Last resort, copy the pixels into memory synchronously. This call may
  // block main render thread.
  ReadPixelsSync(image, temp_argb_frame);
}

void CanvasCaptureHandler::StartVideoCapture(
    const media::VideoCaptureParams& params,
    const media::VideoCapturerSource::VideoCaptureDeliverFrameCB&
        new_frame_callback,
    const media::VideoCapturerSource::RunningCallback& running_callback) {
  DVLOG(3) << __func__ << " requested "
           << media::VideoCaptureFormat::ToString(params.requested_format);
  DCHECK(main_render_thread_checker_.CalledOnValidThread());
  DCHECK(params.requested_format.IsValid());
  capture_format_ = params.requested_format;
  delegate_.reset(new CanvasCaptureHandlerDelegate(new_frame_callback));
  DCHECK(delegate_);
  ask_for_new_frame_ = true;
  running_callback.Run(true);
}

void CanvasCaptureHandler::RequestRefreshFrame() {
  DVLOG(3) << __func__;
  DCHECK(main_render_thread_checker_.CalledOnValidThread());
  if (last_frame_ && delegate_) {
    io_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&CanvasCaptureHandler::CanvasCaptureHandlerDelegate::
                           SendNewFrameOnIOThread,
                       delegate_->GetWeakPtrForIOThread(), last_frame_,
                       base::TimeTicks::Now()));
  }
}

void CanvasCaptureHandler::StopVideoCapture() {
  DVLOG(3) << __func__;
  DCHECK(main_render_thread_checker_.CalledOnValidThread());
  ask_for_new_frame_ = false;
  io_task_runner_->DeleteSoon(FROM_HERE, delegate_.release());
}

void CanvasCaptureHandler::OnContextLost() {
  DCHECK(main_render_thread_checker_.CalledOnValidThread());
  gl_helper_.reset();
}

void CanvasCaptureHandler::SendAsYUVFrame(bool isOpaque,
                                          bool flip,
                                          const uint8_t* source_ptr,
                                          const gfx::Size& image_size,
                                          int stride,
                                          SkColorType source_color_type) {
  DVLOG(4) << __func__;
  DCHECK(main_render_thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("webrtc", "CanvasCaptureHandler::DeliverAsYUVFrame");

  // If this function is called asynchronously, |delegate_| might have been
  // released already in StopVideoCapture().
  if (!delegate_)
    return;

  const base::TimeTicks timestamp = base::TimeTicks::Now();
  scoped_refptr<VideoFrame> video_frame = frame_pool_.CreateFrame(
      isOpaque ? media::PIXEL_FORMAT_I420 : media::PIXEL_FORMAT_YV12A,
      image_size, gfx::Rect(image_size), image_size,
      timestamp - base::TimeTicks());
  DCHECK(video_frame);

  int (*ConvertToI420)(const uint8* src_argb, int src_stride_argb, uint8* dst_y,
                       int dst_stride_y, uint8* dst_u, int dst_stride_u,
                       uint8* dst_v, int dst_stride_v, int width, int height) =
      nullptr;
  switch (source_color_type) {
    case kRGBA_8888_SkColorType:
      ConvertToI420 = libyuv::ABGRToI420;
      break;
    case kBGRA_8888_SkColorType:
      ConvertToI420 = libyuv::ARGBToI420;
      break;
    default:
      NOTIMPLEMENTED() << "Unexpected SkColorType.";
      return;
  }

  if (ConvertToI420(source_ptr, stride,
                    video_frame->visible_data(media::VideoFrame::kYPlane),
                    video_frame->stride(media::VideoFrame::kYPlane),
                    video_frame->visible_data(media::VideoFrame::kUPlane),
                    video_frame->stride(media::VideoFrame::kUPlane),
                    video_frame->visible_data(media::VideoFrame::kVPlane),
                    video_frame->stride(media::VideoFrame::kVPlane),
                    image_size.width(),
                    (flip ? -1 : 1) * image_size.height()) != 0) {
    DLOG(ERROR) << "Couldn't convert to I420";
    return;
  }
  if (!isOpaque) {
    // It is ok to use ARGB function because alpha has the same alignment for
    // both ABGR and ARGB.
    libyuv::ARGBExtractAlpha(
        source_ptr, image_size.width() * kArgbBytesPerPixel,
        video_frame->visible_data(VideoFrame::kAPlane),
        video_frame->stride(VideoFrame::kAPlane), image_size.width(),
        (flip ? -1 : 1) * image_size.height());
  }

  last_frame_ = video_frame;
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&CanvasCaptureHandler::CanvasCaptureHandlerDelegate::
                         SendNewFrameOnIOThread,
                     delegate_->GetWeakPtrForIOThread(), video_frame,
                     timestamp));
}

void CanvasCaptureHandler::ReadPixelsSync(
    sk_sp<SkImage> image,
    const scoped_refptr<media::VideoFrame> temp_frame) {
  const gfx::Size image_size(image->width(), image->height());
  SkImageInfo image_info = SkImageInfo::MakeN32(
      image_size.width(), image_size.height(), kUnpremul_SkAlphaType);
  const int stride = image_size.width() * kArgbBytesPerPixel;
  if (!image->readPixels(image_info,
                         temp_frame->visible_data(VideoFrame::kYPlane), stride,
                         0 /*srcX*/, 0 /*srcY*/)) {
    DLOG(ERROR) << "Couldn't read SkImage using readPixels()";
    return;
  }
  SendAsYUVFrame(image->isOpaque(), false,
                 temp_frame->visible_data(VideoFrame::kYPlane), image_size,
                 stride, kN32_SkColorType);
}

void CanvasCaptureHandler::ReadPixelsAsync(
    sk_sp<SkImage> image,
    const scoped_refptr<VideoFrame> temp_frame,
    bool flip,
    bool success) {
  DCHECK(main_render_thread_checker_.CalledOnValidThread());
  if (!success) {
    DLOG(ERROR) << "Couldn't read SkImage using async callback";
    return;
  }
  SendAsYUVFrame(image->isOpaque(), flip,
                 temp_frame->visible_data(VideoFrame::kYPlane),
                 gfx::Size(image->width(), image->height()),
                 image->width() * kArgbBytesPerPixel, kN32_SkColorType);
}

void CanvasCaptureHandler::AddVideoCapturerSourceToVideoTrack(
    std::unique_ptr<media::VideoCapturerSource> source,
    blink::WebMediaStreamTrack* web_track) {
  std::string str_track_id;
  base::Base64Encode(base::RandBytesAsString(64), &str_track_id);
  const blink::WebString track_id = blink::WebString::FromASCII(str_track_id);
  blink::WebMediaStreamSource webkit_source;
  std::unique_ptr<MediaStreamVideoSource> media_stream_source(
      new MediaStreamVideoCapturerSource(
          MediaStreamSource::SourceStoppedCallback(), std::move(source)));
  webkit_source.Initialize(track_id, blink::WebMediaStreamSource::kTypeVideo,
                           track_id, false);
  webkit_source.SetExtraData(media_stream_source.get());

  web_track->Initialize(webkit_source);
  web_track->SetTrackData(new MediaStreamVideoTrack(
      media_stream_source.release(),
      MediaStreamVideoSource::ConstraintsCallback(), true));
}

void CanvasCaptureHandler::SendNewFrameForTesting(
    sk_sp<SkImage> image,
    blink::WebGraphicsContext3DProvider* fake_impl,
    viz::ContextProvider* test_provider) {
  DCHECK(main_render_thread_checker_.CalledOnValidThread());
  if (!gl_helper_) {
    gl_helper_.reset(new viz::GLHelper(test_provider->ContextGL(),
                                       test_provider->ContextSupport()));
  }
  SendNewFrame(image, fake_impl);
}

}  // namespace content
