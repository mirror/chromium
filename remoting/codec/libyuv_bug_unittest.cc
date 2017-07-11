
#include "testing/gtest/include/gtest/gtest.h"

#include "base/memory/ptr_util.h"
#include "ui/gfx/geometry/size.h"
#include "base/memory/ref_counted.h"
#include "media/base/video_frame.h"

#include "third_party/libyuv/include/libyuv/convert.h"

TEST(LibyuvBugTest, LibyuvBugTest) {
  gfx::Size input_coded_size_(1456, 936);
  scoped_refptr<media::VideoFrame> video_frame = media::VideoFrame::CreateFrame(
      media::VideoPixelFormat::PIXEL_FORMAT_I420, input_coded_size_,
      gfx::Rect(input_coded_size_), input_coded_size_, base::TimeDelta());

  std::vector<uint8_t> copied(media::VideoFrame::AllocationSize(
      media::VideoPixelFormat::PIXEL_FORMAT_I420, input_coded_size_));

  size_t u_offset = media::VideoFrame::PlaneSize(
                        media::VideoPixelFormat::PIXEL_FORMAT_I420,
                        media::VideoFrame::kYPlane, input_coded_size_)
                        .GetArea();
  size_t v_offset =
      u_offset + media::VideoFrame::PlaneSize(
                     media::VideoPixelFormat::PIXEL_FORMAT_I420,
                     media::VideoFrame::kUPlane, input_coded_size_)
                     .GetArea();
  libyuv::I420Copy(
      video_frame->visible_data(media::VideoFrame::kYPlane),
      video_frame->stride(media::VideoFrame::kYPlane),
      video_frame->visible_data(media::VideoFrame::kUPlane),
      video_frame->stride(media::VideoFrame::kUPlane),
      video_frame->visible_data(media::VideoFrame::kVPlane),
      video_frame->stride(media::VideoFrame::kVPlane), copied.data(),
      video_frame->stride(media::VideoFrame::kYPlane), copied.data() + u_offset,
      video_frame->stride(media::VideoFrame::kUPlane), copied.data() + v_offset,
      video_frame->stride(media::VideoFrame::kVPlane),
      input_coded_size_.width(), input_coded_size_.height());
}
