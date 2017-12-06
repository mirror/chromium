// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_AUDIO_DEBUG_RECORDING_WRITER_H_
#define MEDIA_AUDIO_AUDIO_DEBUG_RECORDING_WRITER_H_

#include <memory>

#include "base/atomicops.h"
#include "base/callback.h"
#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "media/base/audio_parameters.h"
#include "media/base/media_export.h"

#include <stdint.h>
#include "base/files/file.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/sequence_checker.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"

namespace base {
class FilePath;
}  // namespace base

namespace media {

class AudioBus;

// Interface for feeding data to a recorder.
class AudioDebugRecorder {
 public:
  virtual ~AudioDebugRecorder() {}

  // If debug recording is enabled, copies audio data and makes sure it's
  // written on the right thread. Otherwise ignores the data. Can be called on
  // any thread.
  virtual void OnData(const AudioBus* source) = 0;
};

// Writes audio data to a 16 bit PCM WAVE file used for debugging purposes. All
// operations are non-blocking.
// Functions are virtual for the purpose of test mocking.
class MEDIA_EXPORT AudioDebugRecordingWriter : public AudioDebugRecorder {
 public:
  AudioDebugRecordingWriter(const AudioParameters& params,
                            base::OnceClosure on_destruction_closure);
  ~AudioDebugRecordingWriter() override;

  // Start debug recording. Must be called before calling OnData() for the first
  // time after creation or StopRecording() call. Can be called on any sequence;
  // OnData() and StopRecording() must be called on the same sequence as
  // Start().
  virtual void StartRecording(const base::FilePath& file_name);

  // Disable debug recording. Must be called to finish recording. Each call to
  // Start() requires a call to Stop(). Will be automatically called on
  // destruction.
  virtual void StopRecording();

  // AudioDebugRecorder implementation. Can be called on any thread.
  // Write |data| to file.
  // Number of channels and sample rate are used from |params|, the other
  // parameters are ignored.
  void OnData(const AudioBus* source) override;

 protected:
  const AudioParameters params_;

 private:
  class AudioFileWriter;

  using AudioFileWriterUniquePtr =
      std::unique_ptr<AudioFileWriter, base::OnTaskRunnerDeleter>;

  // The task runner to do file output operations on.
  const scoped_refptr<base::SequencedTaskRunner> file_task_runner_ =
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN});

  AudioFileWriterUniquePtr file_writer_;

  FRIEND_TEST_ALL_PREFIXES(AudioDebugRecordingWriterTest, EnableDisable);
  FRIEND_TEST_ALL_PREFIXES(AudioDebugRecordingWriterTest, OnData);

  // Used as a flag to indicate if recording is enabled, accessed on different
  // threads.
  base::subtle::Atomic32 recording_enabled_ = 0;

  // Runs in destructor if set.
  base::OnceClosure on_destruction_closure_;

  base::WeakPtrFactory<AudioDebugRecordingWriter> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(AudioDebugRecordingWriter);
};

}  // namespace media

#endif  // MEDIA_AUDIO_AUDIO_DEBUG_RECORDING_WRITER_H_
