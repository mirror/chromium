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

#include "modules/webaudio/DelayDSPKernel.h"
#include <algorithm>
#include "platform/audio/AudioUtilities.h"
#include "platform/wtf/MathExtras.h"

namespace blink {

DelayDSPKernel::DelayDSPKernel(DelayProcessor* processor)
    : AudioDelayDSPKernel(processor, AudioUtilities::kRenderQuantumFrames) {
  DCHECK(processor);
  DCHECK_GT(processor->SampleRate(), 0);
  if (!(processor && processor->SampleRate() > 0))
    return;

  max_delay_time_ = processor->MaxDelayTime();
  DCHECK_GE(max_delay_time_, 0);
  DCHECK(!std::isnan(max_delay_time_));
  if (max_delay_time_ < 0 || std::isnan(max_delay_time_))
    return;

  buffer_.Allocate(
      BufferLengthForDelay(max_delay_time_, processor->SampleRate()));
  buffer_.Zero();
}

bool DelayDSPKernel::HasSampleAccurateValues() {
  return GetDelayProcessor()->DelayTime().HasSampleAccurateValues();
}

void DelayDSPKernel::CalculateSampleAccurateValues(float* delay_times,
                                                   size_t frames_to_process) {
  GetDelayProcessor()->DelayTime().CalculateSampleAccurateValues(
      delay_times, frames_to_process);
}

double DelayDSPKernel::DelayTime(float) {
  return GetDelayProcessor()->DelayTime().FinalValue();
}

void DelayDSPKernel::ProcessOnlyAudioParams(size_t frames_to_process) {
  DCHECK_LE(frames_to_process, AudioUtilities::kRenderQuantumFrames);

  float values[AudioUtilities::kRenderQuantumFrames];

  GetDelayProcessor()->DelayTime().CalculateSampleAccurateValues(
      values, frames_to_process);
}

}  // namespace blink
