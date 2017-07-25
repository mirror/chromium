/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#ifndef AudioResamplerKernel_h
#define AudioResamplerKernel_h

#include "platform/PlatformExport.h"
#include "platform/audio/AudioArray.h"
#include "platform/wtf/Allocator.h"

namespace blink {

class AudioResampler;

// AudioResamplerKernel does resampling on a single mono channel.
// It uses a simple linear interpolation for good performance.

class PLATFORM_EXPORT AudioResamplerKernel {
  USING_FAST_MALLOC(AudioResamplerKernel);
  WTF_MAKE_NONCOPYABLE(AudioResamplerKernel);

 public:
  AudioResamplerKernel(AudioResampler*);

  // GetSourcePointer() should be called each time before Process() is called.
  // Given a number of frames to process (for subsequent call to Process()), it
  // returns a pointer and number_of_source_frames_needed where sample data
  // should be copied.  This sample data provides the input to the resampler
  // when Process() is called.  frames_to_process must be less than or equal to
  // kMaxFramesToProcess.
  float* GetSourcePointer(size_t frames_to_process,
                          size_t* number_of_source_frames_needed);

  // Process() resamples frames_to_process frames from the source into
  // destination.  Each call to Process() must be preceded by a call to
  // GetSourcePointer() so that source input may be supplied.  frames_to_process
  // must be less than or equal to kMaxFramesToProcess.
  void Process(float* destination, size_t frames_to_process);

  // Resets the processing state.
  void Reset();

  static const size_t kMaxFramesToProcess;

 private:
  double Rate() const;

  AudioResampler* resampler_;
  AudioFloatArray source_buffer_;

  // This is a (floating point) read index on the input stream.
  double virtual_read_index_;

  // We need to have continuity from one call of Process() to the next.
  // last_values_ stores the last two sample values from the last call to
  // process().  fill_index_ represents how many buffered samples we have which
  // can be as many as 2.  For the first call to Process() (or after r=Reset())
  // there will be no buffered samples.
  float last_values_[2];
  unsigned fill_index_;
};

}  // namespace blink

#endif  // AudioResamplerKernel_h
