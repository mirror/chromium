// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/watch_time_recorder.h"

#include <stddef.h>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/test/histogram_tester.h"
#include "base/test/test_message_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/ukm/test_ukm_recorder.h"
#include "media/base/watch_time_keys.h"
#include "media/blink/watch_time_reporter.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
const char kTestOrigin[] = "https://test.google.com/";

class WatchTimeRecorderTest : public testing::Test {
 public:
  WatchTimeRecorderTest()
      : histogram_tester_(new base::HistogramTester()),
        test_recorder_(new ukm::TestUkmRecorder()),
        watch_time_keys_(media::GetWatchTimeKeys()),
        watch_time_power_keys_(media::GetWatchTimePowerKeys()),
        mtbr_keys_({media::kMeanTimeBetweenRebuffersAudioSrc,
                    media::kMeanTimeBetweenRebuffersAudioMse,
                    media::kMeanTimeBetweenRebuffersAudioEme,
                    media::kMeanTimeBetweenRebuffersAudioVideoSrc,
                    media::kMeanTimeBetweenRebuffersAudioVideoMse,
                    media::kMeanTimeBetweenRebuffersAudioVideoEme}),
        smooth_keys_({media::kRebuffersCountAudioSrc,
                      media::kRebuffersCountAudioMse,
                      media::kRebuffersCountAudioEme,
                      media::kRebuffersCountAudioVideoSrc,
                      media::kRebuffersCountAudioVideoMse,
                      media::kRebuffersCountAudioVideoEme}) {
    WatchTimeRecorder::CreateWatchTimeRecorderProvider(
        service_manager::BindSourceInfo(), mojo::MakeRequest(&provider_));
  }

  void Initialize(bool has_audio,
                  bool has_video,
                  bool is_mse,
                  bool is_encrypted) {
    wtr_.reset(new media::WatchTimeReporter(
        mojom::PlaybackProperties::New(
            kUnknownAudioCodec, kUnknownVideoCodec, has_audio, has_video,
            is_mse, is_encrypted, false, gfx::Size(800, 600), url::Origin()),
        base::Bind(&WatchTimeRecorderTest::GetCurrentMediaTime,
                   base::Unretained(this)),
        provider_.get()));
    wtr_->set_reporting_interval_for_testing();
  }

  void CycleWatchTimeReporter() {
    base::RunLoop run_loop;
    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                  run_loop.QuitClosure());
    run_loop.Run();
  }

  void ExpectWatchTime(const std::vector<base::StringPiece>& keys,
                       base::TimeDelta value) {
    for (auto key : watch_time_keys_) {
      auto it = std::find(keys.begin(), keys.end(), key);
      if (it == keys.end()) {
        histogram_tester_->ExpectTotalCount(key.as_string(), 0);
      } else {
        histogram_tester_->ExpectUniqueSample(key.as_string(),
                                              value.InMilliseconds(), 1);
      }
    }
  }

  void ExpectHelper(const std::vector<base::StringPiece>& full_key_list,
                    const std::vector<base::StringPiece>& keys,
                    int64_t value) {
    for (auto key : full_key_list) {
      auto it = std::find(keys.begin(), keys.end(), key);
      if (it == keys.end())
        histogram_tester_->ExpectTotalCount(key.as_string(), 0);
      else
        histogram_tester_->ExpectUniqueSample(key.as_string(), value, 1);
    }
  }

  void ExpectMtbrTime(const std::vector<base::StringPiece>& keys,
                      base::TimeDelta value) {
    ExpectHelper(mtbr_keys_, keys, value.InMilliseconds());
  }

  void ExpectZeroRebuffers(const std::vector<base::StringPiece>& keys) {
    ExpectHelper(smooth_keys_, keys, 0);
  }

  void ExpectRebuffers(const std::vector<base::StringPiece>& keys, int count) {
    ExpectHelper(smooth_keys_, keys, count);
  }

  void ExpectUkmWatchTime(size_t entry, size_t size, base::TimeDelta value) {
    ASSERT_LT(entry, test_recorder_->entries_count());

    const auto& metrics_vector = test_recorder_->GetEntry(entry)->metrics;
    ASSERT_EQ(size, metrics_vector.size());

    for (auto& sample : metrics_vector)
      EXPECT_EQ(value.InMilliseconds(), sample->value);
  }

  void ResetHistogramTester() {
    histogram_tester_.reset(new base::HistogramTester());
  }

  MOCK_METHOD0(GetCurrentMediaTime, base::TimeDelta());

 protected:
  mojom::WatchTimeRecorderProvider provider_;
  std::unique_ptr<base::HistogramTester> histogram_tester_;
  std::unique_ptr<ukm::TestUkmRecorder> test_recorder_;
  std::unique_ptr<media::WatchTimeReporter> wtr_;
  const base::flat_set<base::StringPiece> watch_time_keys_;
  const base::flat_set<base::StringPiece> watch_time_power_keys_;
  const std::vector<base::StringPiece> mtbr_keys_;
  const std::vector<base::StringPiece> smooth_keys_;

  DISALLOW_COPY_AND_ASSIGN(WatchTimeRecorderTest);
};

TEST_F(WatchTimeRecorderTest, BasicAudio) {
  constexpr base::TimeDelta kWatchTimeEarly = base::TimeDelta::FromSeconds(5);
  constexpr base::TimeDelta kWatchTimeLate = base::TimeDelta::FromSeconds(10);
  EXPECT_CALL(*this, GetCurrentMediaTime())
      .WillOnce(testing::Return(base::TimeDelta()))
      .WillOnce(testing::Return(kWatchTimeEarly))
      .WillRepeatedly(testing::Return(kWatchTimeLate));
  Initialize(true, false, true, true);
  wtr_->OnPlaying();

  // No log should have been generated yet since the message loop has not had
  // any chance to pump.
  CycleWatchTimeReporter();
  ExpectWatchTime(std::vector<base::StringPiece>(), base::TimeDelta());

  wtr_->OnUnderflow();
  wtr_->OnUnderflow();
  CycleWatchTimeReporter();
  wtr_.reset();

  ExpectWatchTime({media::kWatchTimeAudioAll, media::kWatchTimeAudioMse,
                   media::kWatchTimeAudioEme, media::kWatchTimeAudioAc,
                   media::kWatchTimeAudioNativeControlsOff,
                   media::kWatchTimeAudioEmbeddedExperience},
                  kWatchTimeLate);
  ExpectMtbrTime({media::kMeanTimeBetweenRebuffersAudioMse,
                  media::kMeanTimeBetweenRebuffersAudioEme},
                 kWatchTimeLate / 2);
  ExpectRebuffers(
      {media::kRebuffersCountAudioMse, media::kRebuffersCountAudioEme}, 2);

  ASSERT_EQ(1U, test_recorder_->sources_count());
  ExpectUkmWatchTime(0, 5, kWatchTimeLate);
  EXPECT_TRUE(test_recorder_->GetSourceForUrl(kTestOrigin));
}

TEST_F(WatchTimeRecorderTest, BasicVideo) {
  constexpr base::TimeDelta kWatchTimeEarly = base::TimeDelta::FromSeconds(5);
  constexpr base::TimeDelta kWatchTimeLate = base::TimeDelta::FromSeconds(10);
  EXPECT_CALL(*this, GetCurrentMediaTime())
      .WillOnce(testing::Return(base::TimeDelta()))
      .WillOnce(testing::Return(kWatchTimeEarly))
      .WillRepeatedly(testing::Return(kWatchTimeLate));
  Initialize(true, true, false, true);
  wtr_->OnPlaying();

  // No log should have been generated yet since the message loop has not had
  // any chance to pump.
  CycleWatchTimeReporter();
  ExpectWatchTime(std::vector<base::StringPiece>(), base::TimeDelta());

  wtr_->OnUnderflow();
  wtr_->OnUnderflow();
  CycleWatchTimeReporter();
  wtr_.reset();

  ExpectWatchTime(
      {media::kWatchTimeAudioVideoAll, media::kWatchTimeAudioVideoSrc,
       media::kWatchTimeAudioVideoEme, media::kWatchTimeAudioVideoAc,
       media::kWatchTimeAudioVideoNativeControlsOff,
       media::kWatchTimeAudioVideoDisplayInline,
       media::kWatchTimeAudioVideoEmbeddedExperience},
      kWatchTimeLate);
  ExpectMtbrTime({media::kMeanTimeBetweenRebuffersAudioVideoSrc,
                  media::kMeanTimeBetweenRebuffersAudioVideoEme},
                 kWatchTimeLate / 2);
  ExpectRebuffers({media::kRebuffersCountAudioVideoSrc,
                   media::kRebuffersCountAudioVideoEme},
                  2);

  ASSERT_EQ(1U, test_recorder_->sources_count());
  ExpectUkmWatchTime(0, 6, kWatchTimeLate);
  EXPECT_TRUE(test_recorder_->GetSourceForUrl(kTestOrigin));
}

TEST_F(WatchTimeRecorderTest, BasicPower) {
  constexpr base::TimeDelta kWatchTime1 = base::TimeDelta::FromSeconds(5);
  constexpr base::TimeDelta kWatchTime2 = base::TimeDelta::FromSeconds(10);
  constexpr base::TimeDelta kWatchTime3 = base::TimeDelta::FromSeconds(30);
  EXPECT_CALL(*this, GetCurrentMediaTime())
      .WillOnce(testing::Return(base::TimeDelta()))
      .WillOnce(testing::Return(kWatchTime1))
      .WillOnce(testing::Return(kWatchTime2))
      .WillOnce(testing::Return(kWatchTime2))
      .WillRepeatedly(testing::Return(kWatchTime3));

  Initialize(true, true, false, true);
  wtr_->OnPlaying();
  wtr_->set_is_on_battery_power_for_testing(true);

  // No log should have been generated yet since the message loop has not had
  // any chance to pump.
  CycleWatchTimeReporter();
  ExpectWatchTime(std::vector<base::StringPiece>(), base::TimeDelta());

  CycleWatchTimeReporter();

  // Transition back to AC power, this should generate AC watch time as well.
  wtr_->OnPowerStateChangeForTesting(false);
  CycleWatchTimeReporter();

  // This should finalize the power watch time on battery.
  ExpectWatchTime({media::kWatchTimeAudioVideoBattery}, kWatchTime2);
  ResetHistogramTester();
  wtr_.reset();

  std::vector<base::StringPiece> normal_keys = {
      media::kWatchTimeAudioVideoAll,
      media::kWatchTimeAudioVideoSrc,
      media::kWatchTimeAudioVideoEme,
      media::kWatchTimeAudioVideoNativeControlsOff,
      media::kWatchTimeAudioVideoDisplayInline,
      media::kWatchTimeAudioVideoEmbeddedExperience};

  for (auto key : watch_time_keys_) {
    if (key == media::kWatchTimeAudioVideoAc) {
      histogram_tester_->ExpectUniqueSample(
          key.as_string(), (kWatchTime3 - kWatchTime2).InMilliseconds(), 1);
      continue;
    }

    auto it = std::find(normal_keys.begin(), normal_keys.end(), key);
    if (it == normal_keys.end()) {
      histogram_tester_->ExpectTotalCount(key.as_string(), 0);
    } else {
      histogram_tester_->ExpectUniqueSample(key.as_string(),
                                            kWatchTime3.InMilliseconds(), 1);
    }
  }

  // Each finalize creates a new source and entry. We don't check the URL here
  // since the TestUkmService() helpers DCHECK() a unique URL per source.
  ASSERT_EQ(2U, test_recorder_->sources_count());
  ASSERT_EQ(2U, test_recorder_->entries_count());
  ExpectUkmWatchTime(0, 1, kWatchTime2);
  ExpectZeroRebuffers({media::kRebuffersCountAudioVideoSrc,
                       media::kRebuffersCountAudioVideoEme});

  // Verify Media.WatchTime keys are properly stripped for UKM reporting.
  EXPECT_TRUE(test_recorder_->FindMetric(test_recorder_->GetEntry(0),
                                         "AudioVideo.Battery"));

  // Spot check one of the non-AC keys; this relies on the assumption that the
  // AC metric is not last.
  const auto& metrics_vector = test_recorder_->GetEntry(1)->metrics;
  ASSERT_EQ(6U, metrics_vector.size());
}

TEST_F(WatchTimeRecorderTest, BasicControls) {
  constexpr base::TimeDelta kWatchTime1 = base::TimeDelta::FromSeconds(5);
  constexpr base::TimeDelta kWatchTime2 = base::TimeDelta::FromSeconds(10);
  constexpr base::TimeDelta kWatchTime3 = base::TimeDelta::FromSeconds(30);
  EXPECT_CALL(*this, GetCurrentMediaTime())
      .WillOnce(testing::Return(base::TimeDelta()))
      .WillOnce(testing::Return(kWatchTime1))
      .WillOnce(testing::Return(kWatchTime2))
      .WillOnce(testing::Return(kWatchTime2))
      .WillRepeatedly(testing::Return(kWatchTime3));

  Initialize(true, true, false, true);
  wtr_->OnNativeControlsEnabled();
  wtr_->OnPlaying();

  // No log should have been generated yet since the message loop has not had
  // any chance to pump.
  CycleWatchTimeReporter();
  ExpectWatchTime(std::vector<base::StringPiece>(), base::TimeDelta());

  CycleWatchTimeReporter();

  // Transition back to non-native controls, this should generate controls watch
  // time as well.
  wtr_->OnNativeControlsDisabled();
  CycleWatchTimeReporter();

  // This should finalize the power watch time on native controls.
  ExpectWatchTime({media::kWatchTimeAudioVideoNativeControlsOn}, kWatchTime2);
  ResetHistogramTester();
  wtr_.reset();

  std::vector<base::StringPiece> normal_keys = {
      media::kWatchTimeAudioVideoAll,
      media::kWatchTimeAudioVideoSrc,
      media::kWatchTimeAudioVideoEme,
      media::kWatchTimeAudioVideoAc,
      media::kWatchTimeAudioVideoDisplayInline,
      media::kWatchTimeAudioVideoEmbeddedExperience};

  for (auto key : watch_time_keys_) {
    if (key == media::kWatchTimeAudioVideoNativeControlsOff) {
      histogram_tester_->ExpectUniqueSample(
          key.as_string(), (kWatchTime3 - kWatchTime2).InMilliseconds(), 1);
      continue;
    }

    auto it = std::find(normal_keys.begin(), normal_keys.end(), key);
    if (it == normal_keys.end()) {
      histogram_tester_->ExpectTotalCount(key.as_string(), 0);
    } else {
      histogram_tester_->ExpectUniqueSample(key.as_string(),
                                            kWatchTime3.InMilliseconds(), 1);
    }
  }

  // Each finalize creates a new source and entry. We don't check the URL here
  // since the TestUkmService() helpers DCHECK() a unique URL per source.
  ASSERT_EQ(2U, test_recorder_->sources_count());
  ASSERT_EQ(2U, test_recorder_->entries_count());
  ExpectUkmWatchTime(0, 1, kWatchTime2);

  // Verify Media.WatchTime keys are properly stripped for UKM reporting.
  EXPECT_TRUE(test_recorder_->FindMetric(test_recorder_->GetEntry(0),
                                         "AudioVideo.NativeControlsOn"));

  // Spot check one of the non-AC keys; this relies on the assumption that the
  // AC metric is not last.
  const auto& metrics_vector = test_recorder_->GetEntry(1)->metrics;
  ASSERT_EQ(6U, metrics_vector.size());
  EXPECT_EQ(kWatchTime3.InMilliseconds(), metrics_vector.back()->value);
}

TEST_F(WatchTimeRecorderTest, BasicDisplay) {
  constexpr base::TimeDelta kWatchTime1 = base::TimeDelta::FromSeconds(5);
  constexpr base::TimeDelta kWatchTime2 = base::TimeDelta::FromSeconds(10);
  constexpr base::TimeDelta kWatchTime3 = base::TimeDelta::FromSeconds(30);
  EXPECT_CALL(*this, GetCurrentMediaTime())
      .WillOnce(testing::Return(base::TimeDelta()))
      .WillOnce(testing::Return(kWatchTime1))
      .WillOnce(testing::Return(kWatchTime2))
      .WillOnce(testing::Return(kWatchTime2))
      .WillRepeatedly(testing::Return(kWatchTime3));

  Initialize(true, true, false, true);
  wtr_->OnDisplayTypeFullscreen();
  wtr_->OnPlaying();

  // No log should have been generated yet since the message loop has not had
  // any chance to pump.
  CycleWatchTimeReporter();
  ExpectWatchTime(std::vector<base::StringPiece>(), base::TimeDelta());

  CycleWatchTimeReporter();

  // Transition back to display inline, this should generate controls watch time
  // as well.
  wtr_->OnDisplayTypeInline();
  CycleWatchTimeReporter();

  // This should finalize the power watch time on display type.
  ExpectWatchTime({media::kWatchTimeAudioVideoDisplayFullscreen}, kWatchTime2);
  ResetHistogramTester();
  wtr_.reset();

  std::vector<base::StringPiece> normal_keys = {
      media::kWatchTimeAudioVideoAll,
      media::kWatchTimeAudioVideoSrc,
      media::kWatchTimeAudioVideoEme,
      media::kWatchTimeAudioVideoAc,
      media::kWatchTimeAudioVideoNativeControlsOff,
      media::kWatchTimeAudioVideoEmbeddedExperience};

  for (auto key : watch_time_keys_) {
    if (key == media::kWatchTimeAudioVideoDisplayInline) {
      histogram_tester_->ExpectUniqueSample(
          key.as_string(), (kWatchTime3 - kWatchTime2).InMilliseconds(), 1);
      continue;
    }

    auto it = std::find(normal_keys.begin(), normal_keys.end(), key);
    if (it == normal_keys.end()) {
      histogram_tester_->ExpectTotalCount(key.as_string(), 0);
    } else {
      histogram_tester_->ExpectUniqueSample(key.as_string(),
                                            kWatchTime3.InMilliseconds(), 1);
    }
  }

  // Each finalize creates a new source and entry. We don't check the URL here
  // since the TestUkmService() helpers DCHECK() a unique URL per source.
  ASSERT_EQ(2U, test_recorder_->sources_count());
  ASSERT_EQ(2U, test_recorder_->entries_count());
  ExpectUkmWatchTime(0, 1, kWatchTime2);

  // Verify Media.WatchTime keys are properly stripped for UKM reporting.
  EXPECT_TRUE(test_recorder_->FindMetric(test_recorder_->GetEntry(0),
                                         "AudioVideo.DisplayFullscreen"));

  // Spot check one of the non-AC keys; this relies on the assumption that the
  // AC metric is not last.
  const auto& metrics_vector = test_recorder_->GetEntry(1)->metrics;
  ASSERT_EQ(6U, metrics_vector.size());
  EXPECT_EQ(kWatchTime3.InMilliseconds(), metrics_vector.back()->value);
}

TEST_F(WatchTimeRecorderTest, PowerControlsFinalize) {
  constexpr base::TimeDelta kWatchTime1 = base::TimeDelta::FromSeconds(5);
  constexpr base::TimeDelta kWatchTime2 = base::TimeDelta::FromSeconds(10);
  constexpr base::TimeDelta kWatchTime3 = base::TimeDelta::FromSeconds(30);
  EXPECT_CALL(*this, GetCurrentMediaTime())
      .WillOnce(testing::Return(base::TimeDelta()))
      .WillOnce(testing::Return(kWatchTime1))
      .WillOnce(testing::Return(kWatchTime1))
      .WillOnce(testing::Return(kWatchTime2))
      .WillOnce(testing::Return(kWatchTime2))
      .WillRepeatedly(testing::Return(kWatchTime3));

  Initialize(true, true, false, true);
  wtr_->OnPlaying();

  // No log should have been generated yet since the message loop has not had
  // any chance to pump.
  CycleWatchTimeReporter();
  ExpectWatchTime(std::vector<base::StringPiece>(), base::TimeDelta());

  CycleWatchTimeReporter();

  // Transition controls and power.
  wtr_->OnNativeControlsEnabled();
  wtr_->OnPowerStateChangeForTesting(true);
  CycleWatchTimeReporter();

  // This should finalize the power and controls watch times.
  ExpectWatchTime({media::kWatchTimeAudioVideoNativeControlsOff,
                   media::kWatchTimeAudioVideoAc},
                  kWatchTime2);
  ResetHistogramTester();
  wtr_.reset();
}

TEST_F(WatchTimeRecorderTest, PowerDisplayFinalize) {
  constexpr base::TimeDelta kWatchTime1 = base::TimeDelta::FromSeconds(5);
  constexpr base::TimeDelta kWatchTime2 = base::TimeDelta::FromSeconds(10);
  constexpr base::TimeDelta kWatchTime3 = base::TimeDelta::FromSeconds(30);
  EXPECT_CALL(*this, GetCurrentMediaTime())
      .WillOnce(testing::Return(base::TimeDelta()))
      .WillOnce(testing::Return(kWatchTime1))
      .WillOnce(testing::Return(kWatchTime1))
      .WillOnce(testing::Return(kWatchTime2))
      .WillOnce(testing::Return(kWatchTime2))
      .WillRepeatedly(testing::Return(kWatchTime3));

  Initialize(true, true, false, true);
  wtr_->OnPlaying();

  // No log should have been generated yet since the message loop has not had
  // any chance to pump.
  CycleWatchTimeReporter();
  ExpectWatchTime(std::vector<base::StringPiece>(), base::TimeDelta());

  CycleWatchTimeReporter();

  // Transition display and power.
  wtr_->OnDisplayTypePictureInPicture();
  wtr_->OnPowerStateChangeForTesting(true);
  CycleWatchTimeReporter();

  // This should finalize the power and controls watch times.
  ExpectWatchTime(
      {media::kWatchTimeAudioVideoDisplayInline, media::kWatchTimeAudioVideoAc},
      kWatchTime2);
  ResetHistogramTester();
  wtr_.reset();
}

TEST_F(WatchTimeRecorderTest, PowerDisplayControlsFinalize) {
  constexpr base::TimeDelta kWatchTime1 = base::TimeDelta::FromSeconds(5);
  constexpr base::TimeDelta kWatchTime2 = base::TimeDelta::FromSeconds(10);
  constexpr base::TimeDelta kWatchTime3 = base::TimeDelta::FromSeconds(30);
  EXPECT_CALL(*this, GetCurrentMediaTime())
      .WillOnce(testing::Return(base::TimeDelta()))
      .WillOnce(testing::Return(kWatchTime1))
      .WillOnce(testing::Return(kWatchTime1))
      .WillOnce(testing::Return(kWatchTime2))
      .WillOnce(testing::Return(kWatchTime2))
      .WillOnce(testing::Return(kWatchTime2))
      .WillRepeatedly(testing::Return(kWatchTime3));

  Initialize(true, true, false, true);
  wtr_->OnPlaying();

  // No log should have been generated yet since the message loop has not had
  // any chance to pump.
  CycleWatchTimeReporter();
  ExpectWatchTime(std::vector<base::StringPiece>(), base::TimeDelta());

  CycleWatchTimeReporter();

  // Transition display and power.
  wtr_->OnNativeControlsEnabled();
  wtr_->OnDisplayTypeFullscreen();
  wtr_->OnPowerStateChangeForTesting(true);
  CycleWatchTimeReporter();

  // This should finalize the power and controls watch times.
  ExpectWatchTime({media::kWatchTimeAudioVideoDisplayInline,
                   media::kWatchTimeAudioVideoNativeControlsOff,
                   media::kWatchTimeAudioVideoAc},
                  kWatchTime2);
  ResetHistogramTester();
  wtr_.reset();
}

TEST_F(WatchTimeRecorderTest, BasicHidden) {
  constexpr base::TimeDelta kWatchTimeEarly = base::TimeDelta::FromSeconds(5);
  constexpr base::TimeDelta kWatchTimeLate = base::TimeDelta::FromSeconds(10);
  EXPECT_CALL(*this, GetCurrentMediaTime())
      .WillOnce(testing::Return(base::TimeDelta()))
      .WillOnce(testing::Return(kWatchTimeEarly))
      .WillRepeatedly(testing::Return(kWatchTimeLate));
  Initialize(true, true, false, true);
  wtr_->OnHidden();
  wtr_->OnPlaying();

  // No log should have been generated yet since the message loop has not had
  // any chance to pump.
  CycleWatchTimeReporter();
  ExpectWatchTime(std::vector<base::StringPiece>(), base::TimeDelta());

  CycleWatchTimeReporter();
  wtr_.reset();

  ExpectWatchTime({media::kWatchTimeAudioVideoBackgroundAll,
                   media::kWatchTimeAudioVideoBackgroundSrc,
                   media::kWatchTimeAudioVideoBackgroundEme,
                   media::kWatchTimeAudioVideoBackgroundAc,
                   media::kWatchTimeAudioVideoBackgroundEmbeddedExperience},
                  kWatchTimeLate);

  ASSERT_EQ(1U, test_recorder_->sources_count());
  ExpectUkmWatchTime(0, 4, kWatchTimeLate);
  EXPECT_TRUE(test_recorder_->GetSourceForUrl(kTestOrigin));
}

TEST_F(WatchTimeRecorderTest, FinalizeWithoutWatchTime) {
  EXPECT_CALL(*this, GetCurrentMediaTime())
      .WillRepeatedly(testing::Return(base::TimeDelta()));
  Initialize(true, true, false, true);
  wtr_->OnPlaying();
  wtr_.reset();

  // No watch time should have been recorded even though a finalize event will
  // be sent.
  ExpectWatchTime(std::vector<base::StringPiece>(), base::TimeDelta());
  ExpectMtbrTime(std::vector<base::StringPiece>(), base::TimeDelta());
  ExpectZeroRebuffers(std::vector<base::StringPiece>());
  ASSERT_EQ(0U, test_recorder_->sources_count());
}

TEST_F(WatchTimeRecorderTest, PlayerDestructionFinalizes) {
  constexpr base::TimeDelta kWatchTimeEarly = base::TimeDelta::FromSeconds(5);
  constexpr base::TimeDelta kWatchTimeLate = base::TimeDelta::FromSeconds(10);
  EXPECT_CALL(*this, GetCurrentMediaTime())
      .WillOnce(testing::Return(base::TimeDelta()))
      .WillOnce(testing::Return(kWatchTimeEarly))
      .WillRepeatedly(testing::Return(kWatchTimeLate));
  Initialize(true, true, true, true);
  wtr_->OnPlaying();

  // No log should have been generated yet since the message loop has not had
  // any chance to pump.
  CycleWatchTimeReporter();
  ExpectWatchTime(std::vector<base::StringPiece>(), base::TimeDelta());

  CycleWatchTimeReporter();

  media_log_->AddEvent(
      media_log_->CreateEvent(media::MediaLogEvent::WEBMEDIAPLAYER_DESTROYED));

  ExpectWatchTime(
      {media::kWatchTimeAudioVideoAll, media::kWatchTimeAudioVideoMse,
       media::kWatchTimeAudioVideoEme, media::kWatchTimeAudioVideoAc,
       media::kWatchTimeAudioVideoNativeControlsOff,
       media::kWatchTimeAudioVideoDisplayInline,
       media::kWatchTimeAudioVideoEmbeddedExperience},
      kWatchTimeLate);
  ExpectZeroRebuffers({media::kRebuffersCountAudioVideoMse,
                       media::kRebuffersCountAudioVideoEme});

  ASSERT_EQ(1U, test_recorder_->sources_count());
  ExpectUkmWatchTime(0, 6, kWatchTimeLate);
  EXPECT_TRUE(test_recorder_->GetSourceForUrl(kTestOrigin));
}

TEST_F(WatchTimeRecorderTest, ProcessDestructionFinalizes) {
  constexpr base::TimeDelta kWatchTimeEarly = base::TimeDelta::FromSeconds(5);
  constexpr base::TimeDelta kWatchTimeLate = base::TimeDelta::FromSeconds(10);
  EXPECT_CALL(*this, GetCurrentMediaTime())
      .WillOnce(testing::Return(base::TimeDelta()))
      .WillOnce(testing::Return(kWatchTimeEarly))
      .WillRepeatedly(testing::Return(kWatchTimeLate));
  Initialize(true, true, false, true);
  wtr_->OnPlaying();

  // No log should have been generated yet since the message loop has not had
  // any chance to pump.
  CycleWatchTimeReporter();
  ExpectWatchTime(std::vector<base::StringPiece>(), base::TimeDelta());

  CycleWatchTimeReporter();

  // Also verify that if UKM has already been destructed, we don't crash.
  test_recorder_.reset();
  internals_->OnProcessTerminatedForTesting(render_process_id_);
  ExpectWatchTime(
      {media::kWatchTimeAudioVideoAll, media::kWatchTimeAudioVideoSrc,
       media::kWatchTimeAudioVideoEme, media::kWatchTimeAudioVideoAc,
       media::kWatchTimeAudioVideoNativeControlsOff,
       media::kWatchTimeAudioVideoDisplayInline,
       media::kWatchTimeAudioVideoEmbeddedExperience},
      kWatchTimeLate);
  ExpectZeroRebuffers({media::kRebuffersCountAudioVideoSrc,
                       media::kRebuffersCountAudioVideoEme});
}

}  // namespace
