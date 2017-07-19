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
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

class WatchTimeRecorderTest : public testing::Test {
 public:
  WatchTimeRecorderTest()
      : histogram_tester_(new base::HistogramTester()),
        test_recorder_(new ukm::TestUkmRecorder()),
        // watch_time_keys_(media::GetWatchTimeKeys()),
        // watch_time_power_keys_(media::GetWatchTimePowerKeys()),
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

  ~WatchTimeRecorderTest() override { CycleWatchTimeReporter(); }

  void Initialize(bool has_audio,
                  bool has_video,
                  bool is_mse,
                  bool is_encrypted) {
    provider_->AcquireWatchTimeRecorder(
        mojom::PlaybackProperties::New(
            kUnknownAudioCodec, kUnknownVideoCodec, has_audio, has_video,
            is_mse, is_encrypted, false, gfx::Size(800, 600), url::Origin()),
        mojo::MakeRequest(&wtr_));
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
  base::MessageLoop message_loop_;
  mojom::WatchTimeRecorderProviderPtr provider_;
  std::unique_ptr<base::HistogramTester> histogram_tester_;
  std::unique_ptr<ukm::TestUkmRecorder> test_recorder_;
  mojom::WatchTimeRecorderPtr wtr_;
  const base::flat_set<base::StringPiece> watch_time_keys_;
  const base::flat_set<base::StringPiece> watch_time_power_keys_;
  const std::vector<base::StringPiece> mtbr_keys_;
  const std::vector<base::StringPiece> smooth_keys_;

  DISALLOW_COPY_AND_ASSIGN(WatchTimeRecorderTest);
};

TEST_F(WatchTimeRecorderTest, TestAllKeys) {
  constexpr base::TimeDelta kWatchTime1 = base::TimeDelta::FromSeconds(25);
  constexpr base::TimeDelta kWatchTime2 = base::TimeDelta::FromSeconds(50);
  // Don't include kAudioVideoBackgroundEmbeddedExperience, since we'll use that
  // as a placeholder to ensure only the keys requested for finalize are
  // finalized.
  for (int i = 0;
       i <
       static_cast<int>(WatchTimeKey::kAudioVideoBackgroundEmbeddedExperience);
       ++i) {
    WatchTimeKey key = static_cast<WatchTimeKey>(i);

    Initialize(true, false, true, true);
    wtr_->RecordWatchTime(WatchTimeKey::kAudioVideoBackgroundEmbeddedExperience,
                          kWatchTime1);
    wtr_->RecordWatchTime(key, kWatchTime1);
    wtr_->RecordWatchTime(key, kWatchTime2);

    const int kRebuffers = 2;
    switch (key) {
      case WatchTimeKey::kAudioSrc:
      case WatchTimeKey::kAudioMse:
      case WatchTimeKey::kAudioEme:
      case WatchTimeKey::kAudioVideoSrc:
      case WatchTimeKey::kAudioVideoMse:
      case WatchTimeKey::kAudioVideoEme:
        wtr_->UpdateUnderflowCount(kRebuffers);
      default:
        break;
    }

    CycleWatchTimeReporter();

    // Nothing should be recorded yet since we haven't finalized.
    ExpectWatchTime(std::vector<base::StringPiece>(), base::TimeDelta());

    // Only the requested key should be finalized.
    wtr_->FinalizeWatchTime(std::vector<WatchTimeKey>(1, key));
    CycleWatchTimeReporter();

    ExpectWatchTime(
        std::vector<base::StringPiece>(1, WatchTimeKeyToString(key)),
        kWatchTime2);

    // base::StringPiece mtbr_key, rebuffer_key;
    // switch (key) {
    //   case WatchTimeKey::kAudioSrc:
    //     mtbr_key = kMeanTimeBetweenRebuffersAudioSrc;
    //     rebuffer_key = kRebuffersCountAudioSrc;
    //     break;
    //   case WatchTimeKey::kAudioMse:
    //     mtbr_key = kMeanTimeBetweenRebuffersAudioMse;
    //     rebuffer_key = kRebuffersCountAudioSrc;
    //     break;

    //   case WatchTimeKey::kAudioEme:
    //     mtbr_key = kMeanTimeBetweenRebuffersAudioEme;
    //     rebuffer_key = kRebuffersCountAudioSrc;
    //     break;

    //   case WatchTimeKey::kAudioVideoSrc:
    //     mtbr_key = kMeanTimeBetweenRebuffersAudioVideoSrc;
    //     rebuffer_key = kRebuffersCountAudioSrc;
    //     break;

    //   case WatchTimeKey::kAudioVideoMse:
    //     mtbr_key = kMeanTimeBetweenRebuffersAudioVideoMse;
    //     rebuffer_key = kRebuffersCountAudioSrc;
    //     break;

    //   case WatchTimeKey::kAudioVideoEme:
    //     mtbr_key = kMeanTimeBetweenRebuffersAudioVideoEme;
    //     rebuffer_key = kRebuffersCountAudioSrc;
    //     break;

    //   default:
    //     break;
    // }

    // if (!mtbr_key.empty() && !rebuffer_key.empty()) {
    //   ExpectMtbrTime({mtbr_key}, kWatchTime2 / kRebuffers);
    //   ExpectRebuffers({rebuffer_key}, kRebuffers);
    // }
  }
}

TEST_F(WatchTimeRecorderTest, FinalizeWithoutWatchTime) {
  // EXPECT_CALL(*this, GetCurrentMediaTime())
  //     .WillRepeatedly(testing::Return(base::TimeDelta()));
  // Initialize(true, true, false, true);
  // wtr_->OnPlaying();
  // wtr_.reset();

  // // No watch time should have been recorded even though a finalize event
  // will
  // // be sent.
  // ExpectWatchTime(std::vector<base::StringPiece>(), base::TimeDelta());
  // ExpectMtbrTime(std::vector<base::StringPiece>(), base::TimeDelta());
  // ExpectZeroRebuffers(std::vector<base::StringPiece>());
  // ASSERT_EQ(0U, test_recorder_->sources_count());
}

TEST_F(WatchTimeRecorderTest, PlayerDestructionFinalizes) {
  // constexpr base::TimeDelta kWatchTimeEarly =
  // base::TimeDelta::FromSeconds(5); constexpr base::TimeDelta kWatchTimeLate =
  // base::TimeDelta::FromSeconds(10); EXPECT_CALL(*this, GetCurrentMediaTime())
  //     .WillOnce(testing::Return(base::TimeDelta()))
  //     .WillOnce(testing::Return(kWatchTimeEarly))
  //     .WillRepeatedly(testing::Return(kWatchTimeLate));
  // Initialize(true, true, true, true);
  // wtr_->OnPlaying();

  // // No log should have been generated yet since the message loop has not had
  // // any chance to pump.
  // CycleWatchTimeReporter();
  // ExpectWatchTime(std::vector<base::StringPiece>(), base::TimeDelta());

  // CycleWatchTimeReporter();

  // media_log_->AddEvent(
  //   media_log_->CreateEvent(media::MediaLogEvent::WEBMEDIAPLAYER_DESTROYED));

  // ExpectWatchTime(
  //     {media::kWatchTimeAudioVideoAll, media::kWatchTimeAudioVideoMse,
  //      media::kWatchTimeAudioVideoEme, media::kWatchTimeAudioVideoAc,
  //      media::kWatchTimeAudioVideoNativeControlsOff,
  //      media::kWatchTimeAudioVideoDisplayInline,
  //      media::kWatchTimeAudioVideoEmbeddedExperience},
  //     kWatchTimeLate);
  // ExpectZeroRebuffers({media::kRebuffersCountAudioVideoMse,
  //                      media::kRebuffersCountAudioVideoEme});

  // ASSERT_EQ(1U, test_recorder_->sources_count());
  // ExpectUkmWatchTime(0, 6, kWatchTimeLate);
  // EXPECT_TRUE(test_recorder_->GetSourceForUrl(kTestOrigin));
}

}  // namespace media
