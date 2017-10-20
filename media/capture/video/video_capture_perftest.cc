// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/ref_counted_memory.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_task_environment.h"
#include "base/test/trace_event_analyzer.h"
#include "base/trace_event/trace_event_impl.h"
#include "media/capture/video/fake_video_capture_device_factory.h"
#include "media/capture/video/mock_video_frame_receiver.h"
#include "media/capture/video/video_capture_buffer_pool_impl.h"
#include "media/capture/video/video_capture_buffer_tracker_factory_impl.h"
#include "media/capture/video/video_capture_device_client.h"
#include "media/capture/video/video_capture_jpeg_decoder.h"
#include "media/capture/video/video_frame_receiver.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/perf/perf_test.h"

using trace_analyzer::TraceAnalyzer;
using trace_analyzer::Query;
using trace_analyzer::TraceEventVector;
using ::testing::NiceMock;

namespace media {

class VideoCapturePerfTest : public ::testing::Test {
 public:
  VideoCapturePerfTest() : trace_log_(nullptr) {}
  ~VideoCapturePerfTest() override {}

 protected:
  void StartTracing() {
    CHECK(trace_log_ == nullptr) << "Can only can start tracing once";
    trace_log_ = base::trace_event::TraceLog::GetInstance();
    base::trace_event::TraceConfig trace_config(
        "video", base::trace_event::RECORD_UNTIL_FULL);
    trace_log_->SetEnabled(trace_config,
                           base::trace_event::TraceLog::RECORDING_MODE);
    // Check that we are indeed recording.
    EXPECT_EQ(trace_log_->GetNumTracesRecorded(), 1);
  }

  void StopTracing() {
    trace_log_->SetDisabled();
    base::RunLoop wait_for_flush;
    trace_log_->Flush(base::Bind(&VideoCapturePerfTest::OnTraceDataCollected,
                                 base::Unretained(this),
                                 wait_for_flush.QuitClosure()));
    wait_for_flush.Run();
  }

  TraceAnalyzer* CreateTraceAnalyzer() {
    return TraceAnalyzer::Create("[" + recorded_trace_data_->data() + "]");
  }

 private:
  void OnTraceDataCollected(
      base::OnceClosure done_cb,
      const scoped_refptr<base::RefCountedString>& events_str_ptr,
      bool has_more_events) {
    CHECK(!has_more_events);
    recorded_trace_data_ = events_str_ptr;
    base::ResetAndReturn(&done_cb).Run();
  }

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  base::trace_event::TraceLog* trace_log_;
  scoped_refptr<base::RefCountedString> recorded_trace_data_;
  DISALLOW_COPY_AND_ASSIGN(VideoCapturePerfTest);
};

// Collects trace events that measure how frequently
// VideoCaptureDeviceClient::OnIncomingCapturedData() is called and how long it
// takes until it returns.
TEST_F(VideoCapturePerfTest,
       TraceVideoCaptureDeviceClientOnIncomingCapturedData) {
  const std::string kMeasureFilter =
      "VideoCaptureDeviceClient::OnIncomingCapturedData";
  const std::string kGraphName = "VideoCaptureDeviceClient";

  scoped_refptr<media::VideoCaptureBufferPoolImpl> buffer_pool(
      new media::VideoCaptureBufferPoolImpl(
          base::MakeUnique<media::VideoCaptureBufferTrackerFactoryImpl>(), 3));
  auto controller = base::MakeUnique<NiceMock<MockVideoFrameReceiver>>();
  auto device_client = base::MakeUnique<media::VideoCaptureDeviceClient>(
      std::move(controller), std::move(buffer_pool),
      base::Bind([]() { return std::unique_ptr<VideoCaptureJpegDecoder>(); }));
  FakeVideoCaptureDeviceFactory factory;
  VideoCaptureDeviceDescriptors device_descriptors;
  factory.GetDeviceDescriptors(&device_descriptors);
  ASSERT_GT(device_descriptors.size(), 0u);
  auto device = factory.CreateDevice(device_descriptors[0]);
  media::VideoCaptureParams arbitrary_params;
  device->AllocateAndStart(arbitrary_params, std::move(device_client));

  StartTracing();

  base::RunLoop wait_run_loop;
  const int64_t kRunDurationInSeconds = 10;
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, wait_run_loop.QuitClosure(),
      base::TimeDelta::FromSeconds(kRunDurationInSeconds));
  wait_run_loop.Run();

  StopTracing();

  std::unique_ptr<TraceAnalyzer> analyzer(CreateTraceAnalyzer());
  analyzer->AssociateBeginEndEvents();
  trace_analyzer::TraceEventVector events;
  analyzer->FindEvents(Query::EventNameIs(kMeasureFilter), &events);
  ASSERT_GT(events.size(), 0u)
      << "Could not collect any samples during test, this is bad";

  std::string duration_us;
  std::string interarrival_us;
  for (size_t i = 0; i != events.size(); ++i) {
    duration_us.append(
        base::StringPrintf("%d,", static_cast<int>(events[i]->duration)));
  }

  for (size_t i = 1; i < events.size(); ++i) {
    // The event |timestamp| comes in ns, divide to get us like |duration|.
    interarrival_us.append(base::StringPrintf(
        "%d,",
        static_cast<int>((events[i]->timestamp - events[i - 1]->timestamp) /
                         base::Time::kNanosecondsPerMicrosecond)));
  }

  perf_test::PrintResultList(kGraphName, "", "sample_duration", duration_us,
                             "us", true);

  perf_test::PrintResultList(kGraphName, "", "interarrival_time",
                             interarrival_us, "us", true);
}

}  // namespace media
