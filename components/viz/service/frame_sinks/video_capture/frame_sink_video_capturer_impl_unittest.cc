// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/frame_sinks/video_capture/frame_sink_video_capturer_impl.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "components/viz/common/frame_sinks/begin_frame_args.h"
#include "components/viz/common/frame_sinks/copy_output_request.h"
#include "components/viz/common/frame_sinks/copy_output_result.h"
#include "components/viz/service/frame_sinks/video_capture/frame_sink_video_capturer_manager.h"
#include "components/viz/service/frame_sinks/video_capture/frame_sink_video_consumer.h"
#include "media/base/limits.h"
#include "media/base/video_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/color_space.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

using media::VideoCaptureOracle;
using media::VideoFrame;
using media::VideoFrameMetadata;

using testing::_;
using testing::InvokeWithoutArgs;
using testing::NiceMock;
using testing::Return;

namespace viz {
namespace {

// Dummy frame sink ID.
constexpr FrameSinkId kFrameSinkId = FrameSinkId(1, 1);

// The compositor frame interval.
constexpr base::TimeDelta kVsyncInterval =
    base::TimeDelta::FromSecondsD(1.0 / 60.0);

// The size of the compositor frame sink's Surface.
constexpr gfx::Size kSourceSize = gfx::Size(100, 100);

// The size of the VideoFrames produced by the capturer.
constexpr gfx::Size kCaptureSize = gfx::Size(32, 18);

// The location of the letterboxed content within each VideoFrame. All pixels
// outside of this region should be black.
constexpr gfx::Rect kContentRect = gfx::Rect(6, 0, 18, 18);

class MockFrameSinkManager : public FrameSinkVideoCapturerManager {
 public:
  MOCK_METHOD1(FindCapturableFrameSink,
               CapturableFrameSink*(const FrameSinkId& frame_sink_id));
  MOCK_METHOD1(OnCapturerConnectionLost,
               void(FrameSinkVideoCapturerImpl* capturer));
};

class MockConsumer : public FrameSinkVideoConsumer {
 public:
  MOCK_METHOD3(OnFrameCapturedMock,
               void(scoped_refptr<VideoFrame> frame,
                    const gfx::Rect& update_rect,
                    InFlightFrameDelivery* delivery));
  MOCK_METHOD1(OnTargetLost, void(const FrameSinkId& frame_sink_id));
  MOCK_METHOD0(OnStopped, void());

  int num_frames_received() const { return frames_.size(); }

  scoped_refptr<VideoFrame> TakeFrame(int i) { return std::move(frames_[i]); }

  void SendDoneNotification(int i) { std::move(done_callbacks_[i]).Run(); }

 private:
  void OnFrameCaptured(scoped_refptr<VideoFrame> frame,
                       const gfx::Rect& update_rect,
                       std::unique_ptr<InFlightFrameDelivery> delivery) final {
    ASSERT_TRUE(frame.get());
    EXPECT_EQ(gfx::Rect(kCaptureSize), update_rect);
    ASSERT_TRUE(delivery.get());
    OnFrameCapturedMock(frame, update_rect, delivery.get());
    frames_.push_back(std::move(frame));
    done_callbacks_.push_back(
        base::BindOnce(&InFlightFrameDelivery::Done, base::Passed(&delivery)));
  }

  std::vector<scoped_refptr<VideoFrame>> frames_;
  std::vector<base::OnceClosure> done_callbacks_;
};

class SolidColorI420Result : public CopyOutputResult {
 public:
  SolidColorI420Result(const gfx::Rect rect, uint8_t y, uint8_t u, uint8_t v)
      : CopyOutputResult(CopyOutputResult::Format::I420_PLANES, rect),
        y_(y),
        u_(u),
        v_(v) {}

  bool ReadI420Planes(uint8_t* y_out,
                      int y_out_stride,
                      uint8_t* u_out,
                      int u_out_stride,
                      uint8_t* v_out,
                      int v_out_stride) const final {
    CHECK(y_out);
    CHECK(y_out_stride >= size().width());
    CHECK(u_out);
    const int chroma_width = (size().width() + 1) / 2;
    CHECK(u_out_stride >= chroma_width);
    CHECK(v_out);
    CHECK(v_out_stride >= chroma_width);
    for (int i = 0; i < size().height(); ++i, y_out += y_out_stride) {
      memset(y_out, y_, size().width());
    }
    const int chroma_height = (size().height() + 1) / 2;
    for (int i = 0; i < chroma_height; ++i, u_out += u_out_stride) {
      memset(u_out, u_, chroma_width);
    }
    for (int i = 0; i < chroma_height; ++i, v_out += v_out_stride) {
      memset(v_out, v_, chroma_width);
    }
    return true;
  }

 private:
  const uint8_t y_;
  const uint8_t u_;
  const uint8_t v_;
};

class FakeCapturableFrameSink : public CapturableFrameSink {
 public:
  Client* attached_client() const { return client_; }

  void AttachCaptureClient(Client* client) override {
    ASSERT_FALSE(client_);
    ASSERT_TRUE(client);
    client_ = client;
  }

  void DetachCaptureClient(Client* client) override {
    ASSERT_TRUE(client);
    ASSERT_EQ(client, client_);
    client_ = nullptr;
  }

  gfx::Size GetSurfaceSize() override { return kSourceSize; }

  void RequestCopyOfSurface(
      std::unique_ptr<CopyOutputRequest> request) override {
    EXPECT_EQ(CopyOutputResult::Format::I420_PLANES, request->result_format());
    EXPECT_NE(base::UnguessableToken(), request->source());
    EXPECT_EQ(gfx::Rect(kSourceSize), request->area());
    EXPECT_EQ(gfx::Rect(kContentRect.size()), request->result_selection());

    auto result = std::make_unique<SolidColorI420Result>(
        request->result_selection(), y_, u_, v_);
    results_.push_back(base::BindOnce(
        [](std::unique_ptr<CopyOutputRequest> request,
           std::unique_ptr<CopyOutputResult> result) {
          request->SendResult(std::move(result));
        },
        base::Passed(&request), base::Passed(&result)));
  }

  void SetCopyOutputColor(uint8_t y, uint8_t u, uint8_t v) {
    y_ = y;
    u_ = u;
    v_ = v;
  }

  int num_copy_results() const { return results_.size(); }

  void SendCopyOutputResult(int offset) {
    auto it = results_.begin() + offset;
    std::move(*it).Run();
  }

 private:
  CapturableFrameSink::Client* client_ = nullptr;
  uint8_t y_;
  uint8_t u_;
  uint8_t v_;

  std::vector<base::OnceClosure> results_;
};

// Matcher that returns true if the content region of a letterboxed VideoFrame
// is filled with the given color, and black everywhere else.
MATCHER_P3(IsLetterboxedFrame, y, u, v, "") {
  if (!arg) {
    return false;
  }

  const VideoFrame& frame = *arg;
  const auto IsLetterboxedPlane = [&frame](int plane, uint8_t color) {
    gfx::Rect content_rect = kContentRect;
    if (plane != VideoFrame::kYPlane) {
      content_rect =
          gfx::Rect(content_rect.x() / 2, content_rect.y() / 2,
                    content_rect.width() / 2, content_rect.height() / 2);
    }
    for (int row = 0; row < frame.rows(plane); ++row) {
      const uint8_t* p = frame.visible_data(plane) + row * frame.stride(plane);
      for (int col = 0; col < frame.row_bytes(plane); ++col) {
        if (content_rect.Contains(gfx::Point(col, row))) {
          if (p[col] != color) {
            return false;
          }
        } else {  // Letterbox border around content.
          if (plane == VideoFrame::kYPlane && p[col] != 0x00) {
            return false;
          }
        }
      }
    }
    return true;
  };

  return IsLetterboxedPlane(VideoFrame::kYPlane, y) &&
         IsLetterboxedPlane(VideoFrame::kUPlane, u) &&
         IsLetterboxedPlane(VideoFrame::kVPlane, v);
}

}  // namespace

class FrameSinkVideoCapturerTest : public testing::Test {
 public:
  FrameSinkVideoCapturerTest() : capturer_(&frame_sink_manager_) {}

  void SetUp() override {
    // Before setting the format, ensure the defaults are in-place. Then, for
    // these tests, set a specific format and color space.
    ASSERT_EQ(FrameSinkVideoCapturerImpl::kDefaultPixelFormat,
              capturer_.pixel_format_);
    ASSERT_EQ(FrameSinkVideoCapturerImpl::kDefaultColorSpace,
              capturer_.color_space_);
    capturer_.SetFormat(media::PIXEL_FORMAT_I420, media::COLOR_SPACE_HD_REC709);
    ASSERT_EQ(media::PIXEL_FORMAT_I420, capturer_.pixel_format_);
    ASSERT_EQ(media::COLOR_SPACE_HD_REC709, capturer_.color_space_);

    // Set min capture period as small as possible so that the
    // media::VideoCapturerOracle used by the capturer will want to capture
    // every composited frame. The capturer will override the too-small value of
    // zero with a value based on media::limits::kMaxFramesPerSecond.
    capturer_.SetMinCapturePeriod(base::TimeDelta());
    ASSERT_LT(base::TimeDelta(), capturer_.oracle_.min_capture_period());

    capturer_.SetResolutionConstraints(kCaptureSize, kCaptureSize, false);
  }

  base::TimeTicks GetBeginFrameTime(int frame_number) {
    if (!first_begin_frame_time_) {
      first_begin_frame_time_ = base::TimeTicks::Now();
    }
    return *first_begin_frame_time_ + frame_number * kVsyncInterval;
  }

  void NotifyBeginFrame(int frame_number) {
    BeginFrameArgs args;
    args.interval = kVsyncInterval;
    args.frame_time = GetBeginFrameTime(frame_number);
    args.sequence_number = BeginFrameArgs::kStartingFrameNumber + frame_number;
    args.source_id = 1;
    capturer_.OnBeginFrame(args);
  }

  void NotifyFrameDamaged(int frame_number) {
    BeginFrameAck ack;
    ack.sequence_number = BeginFrameArgs::kStartingFrameNumber + frame_number;
    ack.source_id = 1;
    capturer_.OnFrameDamaged(ack, kSourceSize, gfx::Rect(kSourceSize));
  }

 protected:
  MockFrameSinkManager frame_sink_manager_;
  FakeCapturableFrameSink frame_sink_;
  FrameSinkVideoCapturerImpl capturer_;
  base::Optional<base::TimeTicks> first_begin_frame_time_;
};

// Tests that the capturer attaches to a frame sink immediately, in the case
// where the frame sink was already known by the manger.
TEST_F(FrameSinkVideoCapturerTest, ResolvesTargetImmediately) {
  EXPECT_CALL(frame_sink_manager_, FindCapturableFrameSink(kFrameSinkId))
      .WillRepeatedly(Return(&frame_sink_));

  EXPECT_EQ(FrameSinkId(), capturer_.requested_target());
  capturer_.ChangeTarget(kFrameSinkId);
  EXPECT_EQ(kFrameSinkId, capturer_.requested_target());
  EXPECT_EQ(&capturer_, frame_sink_.attached_client());
}

// Tests that the capturer attaches to a frame sink later, in the case where the
// frame sink becomes known to the manger at some later point.
TEST_F(FrameSinkVideoCapturerTest, ResolvesTargetLater) {
  EXPECT_CALL(frame_sink_manager_, FindCapturableFrameSink(kFrameSinkId))
      .WillRepeatedly(Return(nullptr));

  EXPECT_EQ(FrameSinkId(), capturer_.requested_target());
  capturer_.ChangeTarget(kFrameSinkId);
  EXPECT_EQ(kFrameSinkId, capturer_.requested_target());
  EXPECT_EQ(nullptr, frame_sink_.attached_client());

  capturer_.SetResolvedTarget(&frame_sink_);
  EXPECT_EQ(&capturer_, frame_sink_.attached_client());
}

// Tests that the capturer reports a lost target to the consumer. The consumer
// may then change targets to capture something else.
TEST_F(FrameSinkVideoCapturerTest, ReportsTargetLost) {
  FakeCapturableFrameSink other_frame_sink;
  constexpr FrameSinkId kOtherFrameSinkId = FrameSinkId(1, 2);
  EXPECT_CALL(frame_sink_manager_, FindCapturableFrameSink(kOtherFrameSinkId))
      .WillOnce(Return(&other_frame_sink));
  EXPECT_CALL(frame_sink_manager_, FindCapturableFrameSink(kFrameSinkId))
      .WillOnce(Return(&frame_sink_));

  auto consumer = std::make_unique<NiceMock<MockConsumer>>();
  EXPECT_CALL(*consumer, OnTargetLost(kOtherFrameSinkId)).Times(1);
  EXPECT_CALL(*consumer, OnStopped()).Times(1);
  capturer_.Start(std::move(consumer));

  capturer_.ChangeTarget(kOtherFrameSinkId);
  EXPECT_EQ(kOtherFrameSinkId, capturer_.requested_target());
  EXPECT_EQ(&capturer_, other_frame_sink.attached_client());

  capturer_.OnTargetWillGoAway();
  EXPECT_EQ(nullptr, other_frame_sink.attached_client());

  capturer_.ChangeTarget(kFrameSinkId);
  EXPECT_EQ(kFrameSinkId, capturer_.requested_target());
  EXPECT_EQ(&capturer_, frame_sink_.attached_client());
  EXPECT_EQ(nullptr, other_frame_sink.attached_client());

  capturer_.Stop();
}

// Tests that an initial black frame is sent, in the case where a target is not
// resolved at the time Start() is called.
TEST_F(FrameSinkVideoCapturerTest, SendsBlackFrameOnStartWithoutATarget) {
  EXPECT_CALL(frame_sink_manager_, FindCapturableFrameSink(kFrameSinkId))
      .WillRepeatedly(Return(&frame_sink_));

  auto consumer = std::make_unique<MockConsumer>();
  EXPECT_CALL(*consumer,
              OnFrameCapturedMock(IsLetterboxedFrame(0x00, 0x80, 0x80), _, _))
      .Times(1);
  EXPECT_CALL(*consumer, OnStopped()).Times(1);

  capturer_.Start(std::move(consumer));
  EXPECT_EQ(0, frame_sink_.num_copy_results());
  capturer_.Stop();
}

// An end-to-end pipeline test where compositor updates trigger the capturer to
// make copy requests, and a stream of video frames are delivered to the
// consumer.
TEST_F(FrameSinkVideoCapturerTest, CapturesCompositedFrames) {
  frame_sink_.SetCopyOutputColor(0x80, 0x80, 0x80);
  EXPECT_CALL(frame_sink_manager_, FindCapturableFrameSink(kFrameSinkId))
      .WillRepeatedly(Return(&frame_sink_));

  capturer_.ChangeTarget(kFrameSinkId);

  MockConsumer* consumer;
  {
    auto mock_consumer = std::make_unique<MockConsumer>();
    consumer = mock_consumer.get();
    capturer_.Start(std::move(mock_consumer));
  }
  const int num_refresh_frames = 1;
  const int num_composites =
      3 * FrameSinkVideoCapturerImpl::kDesignLimitMaxFrames;
  EXPECT_CALL(*consumer, OnFrameCapturedMock(_, _, _))
      .Times(num_refresh_frames + num_composites);
  EXPECT_CALL(*consumer, OnTargetLost(_)).Times(0);
  EXPECT_CALL(*consumer, OnStopped()).Times(1);

  // To start, the capturer will make a copy request for the initial refresh
  // frame. Simulate a copy result and expect to see the refresh frame delivered
  // to the consumer.
  ASSERT_EQ(1, frame_sink_.num_copy_results());
  frame_sink_.SendCopyOutputResult(0);
  ASSERT_EQ(1, consumer->num_frames_received());
  EXPECT_THAT(consumer->TakeFrame(0), IsLetterboxedFrame(0x80, 0x80, 0x80));
  consumer->SendDoneNotification(0);

  // Drive the capturer pipeline for a series of frame composites.
  base::TimeDelta last_timestamp;
  for (int i = 1; i <= num_composites; ++i) {
    SCOPED_TRACE(testing::Message() << "frame #" << i);

    // Notify the capturer that compositing of the frame has begun.
    NotifyBeginFrame(i);

    // Change the content of the frame sink and notify the capturer of the
    // damage.
    const uint8_t color[3] = {i << 4, (i << 4) + 0x10, (i << 4) + 0x20};
    frame_sink_.SetCopyOutputColor(color[0], color[1], color[2]);
    NotifyFrameDamaged(i);

    // The frame sink should have received a CopyOutputRequest. Simulate a
    // result sent back to the capturer, and the capturer should then deliver
    // the frame.
    ASSERT_EQ(1 + i, frame_sink_.num_copy_results());
    frame_sink_.SendCopyOutputResult(i);
    ASSERT_EQ(1 + i, consumer->num_frames_received());

    // Verify the frame is the right size, has the right content, and has
    // required metadata set.
    const scoped_refptr<VideoFrame> frame = consumer->TakeFrame(i);
    EXPECT_THAT(frame, IsLetterboxedFrame(color[0], color[1], color[2]));
    EXPECT_EQ(kCaptureSize, frame->coded_size());
    EXPECT_EQ(gfx::Rect(kCaptureSize), frame->visible_rect());
    EXPECT_EQ(gfx::ColorSpace::CreateREC709(), frame->ColorSpace());
    EXPECT_LT(last_timestamp, frame->timestamp());
    last_timestamp = frame->timestamp();
    const VideoFrameMetadata* metadata = frame->metadata();
    EXPECT_TRUE(metadata->HasKey(VideoFrameMetadata::CAPTURE_BEGIN_TIME));
    EXPECT_TRUE(metadata->HasKey(VideoFrameMetadata::CAPTURE_END_TIME));
    int color_space = media::COLOR_SPACE_UNSPECIFIED;
    EXPECT_TRUE(
        metadata->GetInteger(VideoFrameMetadata::COLOR_SPACE, &color_space));
    EXPECT_EQ(media::COLOR_SPACE_HD_REC709, color_space);
    EXPECT_TRUE(metadata->HasKey(VideoFrameMetadata::FRAME_DURATION));
    // FRAME_DURATION is an estimate computed by the VideoCaptureOracle, so it
    // its exact value is not being tested here.
    double frame_rate = 0.0;
    EXPECT_TRUE(
        metadata->GetDouble(VideoFrameMetadata::FRAME_RATE, &frame_rate));
    EXPECT_NEAR(media::limits::kMaxFramesPerSecond, frame_rate, 0.001);
    base::TimeTicks reference_time;
    EXPECT_TRUE(metadata->GetTimeTicks(VideoFrameMetadata::REFERENCE_TIME,
                                       &reference_time));
    EXPECT_EQ(GetBeginFrameTime(i) + kVsyncInterval, reference_time);

    // Notify the capturer that the consumer is done with the frame.
    consumer->SendDoneNotification(i);

    if (HasFailure()) {
      break;
    }
  }

  capturer_.Stop();
}

TEST_F(FrameSinkVideoCapturerTest, HaltsWhenPipelineIsFull) {}

TEST_F(FrameSinkVideoCapturerTest, DeliversFramesInOrder) {}

TEST_F(FrameSinkVideoCapturerTest, CancelsInFlightCapturesOnStop) {
  /// and test that next start() does deliver frames...
}

}  // namespace viz
