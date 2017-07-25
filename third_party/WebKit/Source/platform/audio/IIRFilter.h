// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IIRFilter_h
#define IIRFilter_h

#include "platform/PlatformExport.h"
#include "platform/audio/AudioArray.h"
#include "platform/wtf/Vector.h"

namespace blink {

class PLATFORM_EXPORT IIRFilter final {
 public:
  // The maximum IIR filter order.  This also limits the number of feedforward
  // coefficients.  The maximum number of coefficients is 20 according to the
  // spec.
  const static size_t kMaxOrder = 19;
  IIRFilter(const AudioDoubleArray* feedforward_coef,
            const AudioDoubleArray* feedback_coef);
  ~IIRFilter();

  void Process(const float* source_p, float* dest_p, size_t frames_to_process);

  void Reset();

  void GetFrequencyResponse(int n_frequencies,
                            const float* frequency,
                            float* mag_response,
                            float* phase_response);

  // Compute the tail time of the IIR filter
  double TailTime(double sample_rate);

  // Reset the internal state of the IIR filter to the initial state.
  void ResetState();

 private:
  // Filter memory
  //
  // For simplicity, we assume |x_buffer_| and |y_buffer_| have the same length,
  // and the length is a power of two.  Since the number of coefficients has a
  // fixed upper length, the size of x_buffer_ and y_buffer_ is fixed.
  // |x_buffer_| holds the old input values and |y_buffer_| holds the old output
  // values needed to compute the new output value.
  //
  // y_buffer_[buffer_index_] holds the most recent output value, say, y[n].
  // Then y_buffer_[buffer_index_ - k] is y[n - k].  Similarly for x_buffer_.
  //
  // To minimize roundoff, these arrays are double's instead of floats.
  AudioDoubleArray x_buffer_;
  AudioDoubleArray y_buffer_;

  // Index into the x_buffer_ and y_buffer_ arrays where the most current x and
  // y values should be stored.  x_buffer_[buffer_index_] corresponds to x[n],
  // the current x input value and y_buffer_[buffer_index_] is where y[n], the
  // current output value.
  int buffer_index_;

  // Coefficients of the IIR filter.  To minimize storage, these point to the
  // arrays given in the constructor.
  const AudioDoubleArray* feedback_;
  const AudioDoubleArray* feedforward_;
};

}  // namespace blink

#endif  // IIRFilter_h
