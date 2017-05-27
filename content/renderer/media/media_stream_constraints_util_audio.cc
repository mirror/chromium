// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/media_stream_constraints_util_audio.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

#include "content/common/media/media_stream_options.h"
#include "content/renderer/media/media_stream_constraints_util_sets.h"
#include "content/renderer/media/media_stream_video_source.h"
#include "media/base/limits.h"
#include "third_party/WebKit/public/platform/WebMediaConstraints.h"
#include "third_party/WebKit/public/platform/WebString.h"

namespace content {

namespace {

class AudioDeviceInfo {
 public:
  // This constructor is intended for device capture.
  explicit AudioDeviceInfo(
      const ::mojom::AudioInputDeviceCapabilitiesPtr& device_info)
      : device_id_(device_info->device_id),
        parameters_(device_info->parameters) {
    DCHECK(parameters_.IsValid());
  }

  // This constructor is intended for content capture, where constraints on
  // audio parameters are not supported.
  explicit AudioDeviceInfo(std::string device_id)
      : device_id_(std::move(device_id)) {}

  bool operator==(const AudioDeviceInfo& other) const {
    return device_id_ == other.device_id_;
  }

  // Accessors
  const std::string& device_id() const { return device_id_; }
  const media::AudioParameters& parameters() const { return parameters_; }

  // Convenience accessors
  int SampleRate() const {
    DCHECK(parameters_.IsValid());
    return parameters_.sample_rate();
  }
  int SampleSize() const {
    DCHECK(parameters_.IsValid());
    return parameters_.bits_per_sample();
  }
  int ChannelCount() const {
    DCHECK(parameters_.IsValid());
    return parameters_.channels();
  }
  int Effects() const {
    DCHECK(parameters_.IsValid());
    return parameters_.effects();
  }

 private:
  std::string device_id_;
  media::AudioParameters parameters_;
};

using AudioDeviceSet = DiscreteSet<AudioDeviceInfo>;

AudioDeviceSet AudioDeviceSetForDeviceCapture(
    const blink::WebMediaTrackConstraintSet& constraint_set,
    const AudioDeviceCaptureCapabilities& capabilities,
    const char** failed_constraint_name = nullptr) {
  std::vector<AudioDeviceInfo> result;
  for (auto& device_capabilities : capabilities) {
    if (!constraint_set.device_id.Matches(
            blink::WebString::FromASCII(device_capabilities->device_id))) {
      if (failed_constraint_name)
        *failed_constraint_name = constraint_set.device_id.GetName();
      continue;
    }
    if ((ConstraintHasMin(constraint_set.sample_rate) &&
         device_capabilities->parameters.sample_rate() <
             ConstraintMin(constraint_set.sample_rate)) ||
        (ConstraintHasMax(constraint_set.sample_rate) &&
         device_capabilities->parameters.sample_rate() >
             ConstraintMax(constraint_set.sample_rate))) {
      if (failed_constraint_name)
        *failed_constraint_name = constraint_set.sample_rate.GetName();
      continue;
    }
    if ((ConstraintHasMin(constraint_set.sample_size) &&
         device_capabilities->parameters.bits_per_sample() <
             ConstraintMin(constraint_set.sample_size)) ||
        (ConstraintHasMax(constraint_set.sample_size) &&
         device_capabilities->parameters.bits_per_sample() >
             ConstraintMax(constraint_set.sample_size))) {
      if (failed_constraint_name)
        *failed_constraint_name = constraint_set.sample_size.GetName();
      continue;
    }
    if ((ConstraintHasMin(constraint_set.channel_count) &&
         device_capabilities->parameters.channels() <
             ConstraintMin(constraint_set.channel_count)) ||
        (ConstraintHasMax(constraint_set.channel_count) &&
         device_capabilities->parameters.channels() >
             ConstraintMax(constraint_set.channel_count))) {
      if (failed_constraint_name)
        *failed_constraint_name = constraint_set.channel_count.GetName();
      continue;
    }
    result.push_back(AudioDeviceInfo(device_capabilities));
  }
  if (failed_constraint_name && !result.empty())
    *failed_constraint_name = nullptr;

  return AudioDeviceSet(result);
}

AudioDeviceSet AudioDeviceSetForContentCapture(
    const blink::WebMediaTrackConstraintSet& constraint_set,
    const char** failed_constraint_name = nullptr) {
  if (!constraint_set.device_id.HasExact())
    return AudioDeviceSet::UniversalSet();

  std::vector<AudioDeviceInfo> result;
  for (auto& device_id : constraint_set.device_id.Exact())
    result.push_back(AudioDeviceInfo(device_id.Utf8()));

  return AudioDeviceSet(result);
}

class AudioDeviceCaptureCandidates {
 public:
  AudioDeviceCaptureCandidates()
      : failed_constraint_name_(nullptr),
        audio_device_set_(AudioDeviceSet::UniversalSet()),
        hotword_enabled_set_(BoolSet::UniversalSet()),
        disable_local_echo_set_(BoolSet::UniversalSet()),
        render_to_associated_sink_set_(BoolSet::UniversalSet()),
        echo_cancellation_set_(BoolSet::UniversalSet()),
        goog_echo_cancellation_set_(BoolSet::UniversalSet()),
        goog_da_echo_cancellation_set_(BoolSet::UniversalSet()),
        goog_audio_mirroring_set_(BoolSet::UniversalSet()),
        goog_auto_gain_control_set_(BoolSet::UniversalSet()),
        goog_experimental_echo_cancellation_set_(BoolSet::UniversalSet()),
        goog_typing_noise_detection_set_(BoolSet::UniversalSet()),
        goog_noise_suppression_set_(BoolSet::UniversalSet()),
        goog_experimental_noise_suppression_set_(BoolSet::UniversalSet()),
        goog_beamforming_set_(BoolSet::UniversalSet()),
        goog_highpass_filter_set_(BoolSet::UniversalSet()),
        goog_experimental_auto_gain_control_set_(BoolSet::UniversalSet()),
        goog_array_geometry_set_(StringSet::UniversalSet()) {}

  explicit AudioDeviceCaptureCandidates(
      const blink::WebMediaTrackConstraintSet& constraint_set,
      const AudioDeviceCaptureCapabilities& capabilities,
      bool is_device_capture)
      : failed_constraint_name_(nullptr),
        audio_device_set_(
            is_device_capture
                ? AudioDeviceSetForDeviceCapture(constraint_set,
                                                 capabilities,
                                                 &failed_constraint_name_)
                : AudioDeviceSetForContentCapture(constraint_set,
                                                  &failed_constraint_name_)),
        hotword_enabled_set_(
            BoolSetFromConstraint(constraint_set.hotword_enabled)),
        disable_local_echo_set_(
            BoolSetFromConstraint(constraint_set.disable_local_echo)),
        render_to_associated_sink_set_(
            BoolSetFromConstraint(constraint_set.render_to_associated_sink)),
        echo_cancellation_set_(
            BoolSetFromConstraint(constraint_set.echo_cancellation)),
        goog_echo_cancellation_set_(
            BoolSetFromConstraint(constraint_set.goog_echo_cancellation)),
        goog_da_echo_cancellation_set_(
            BoolSetFromConstraint(constraint_set.goog_da_echo_cancellation)),
        goog_audio_mirroring_set_(
            BoolSetFromConstraint(constraint_set.goog_audio_mirroring)),
        goog_auto_gain_control_set_(
            BoolSetFromConstraint(constraint_set.goog_auto_gain_control)),
        goog_experimental_echo_cancellation_set_(BoolSetFromConstraint(
            constraint_set.goog_experimental_echo_cancellation)),
        goog_typing_noise_detection_set_(
            BoolSetFromConstraint(constraint_set.goog_typing_noise_detection)),
        goog_noise_suppression_set_(
            BoolSetFromConstraint(constraint_set.goog_noise_suppression)),
        goog_experimental_noise_suppression_set_(BoolSetFromConstraint(
            constraint_set.goog_experimental_noise_suppression)),
        goog_beamforming_set_(
            BoolSetFromConstraint(constraint_set.goog_beamforming)),
        goog_highpass_filter_set_(
            BoolSetFromConstraint(constraint_set.goog_highpass_filter)),
        goog_experimental_auto_gain_control_set_(BoolSetFromConstraint(
            constraint_set.goog_experimental_auto_gain_control)),
        goog_array_geometry_set_(
            StringSetFromConstraint(constraint_set.goog_array_geometry)) {
    MaybeUpdateFailedNonDeviceConstraintName();
  }

  bool IsEmpty() const { return failed_constraint_name_ != nullptr; }

  AudioDeviceCaptureCandidates Intersection(
      const AudioDeviceCaptureCandidates& other) {
    AudioDeviceCaptureCandidates intersection;
    intersection.audio_device_set_ =
        audio_device_set_.Intersection(other.audio_device_set_);
    if (intersection.audio_device_set_.IsEmpty()) {
      // To mark the intersection as empty, it is necessary to assign a
      // a non-null value to |failed_constraint_name_|. The specific value
      // for an intersection does not actually matter, since the intersection
      // is discarded if empty.
      intersection.failed_constraint_name_ = "some device constraint";
      return intersection;
    }

    intersection.hotword_enabled_set_ =
        hotword_enabled_set_.Intersection(other.hotword_enabled_set_);
    intersection.disable_local_echo_set_ =
        disable_local_echo_set_.Intersection(other.disable_local_echo_set_);
    intersection.render_to_associated_sink_set_ =
        render_to_associated_sink_set_.Intersection(
            other.render_to_associated_sink_set_);
    intersection.echo_cancellation_set_ =
        echo_cancellation_set_.Intersection(other.echo_cancellation_set_);
    intersection.goog_echo_cancellation_set_ =
        goog_echo_cancellation_set_.Intersection(
            other.goog_echo_cancellation_set_);
    intersection.goog_da_echo_cancellation_set_ =
        goog_da_echo_cancellation_set_.Intersection(
            other.goog_da_echo_cancellation_set_);

    // Constraints that control audio-processing behavior.
    intersection.goog_audio_mirroring_set_ =
        goog_audio_mirroring_set_.Intersection(other.goog_audio_mirroring_set_);
    intersection.goog_auto_gain_control_set_ =
        goog_auto_gain_control_set_.Intersection(
            other.goog_auto_gain_control_set_);
    intersection.goog_experimental_echo_cancellation_set_ =
        goog_experimental_echo_cancellation_set_.Intersection(
            other.goog_experimental_echo_cancellation_set_);
    intersection.goog_typing_noise_detection_set_ =
        goog_typing_noise_detection_set_.Intersection(
            other.goog_typing_noise_detection_set_);
    intersection.goog_noise_suppression_set_ =
        goog_noise_suppression_set_.Intersection(
            other.goog_noise_suppression_set_);
    intersection.goog_experimental_noise_suppression_set_ =
        goog_experimental_noise_suppression_set_.Intersection(
            other.goog_experimental_noise_suppression_set_);
    intersection.goog_beamforming_set_ =
        goog_beamforming_set_.Intersection(other.goog_beamforming_set_);
    intersection.goog_highpass_filter_set_ =
        goog_highpass_filter_set_.Intersection(other.goog_highpass_filter_set_);
    intersection.goog_experimental_auto_gain_control_set_ =
        goog_experimental_auto_gain_control_set_.Intersection(
            other.goog_experimental_auto_gain_control_set_);
    intersection.goog_array_geometry_set_ =
        goog_array_geometry_set_.Intersection(other.goog_array_geometry_set_);

    intersection.MaybeUpdateFailedNonDeviceConstraintName();
    return intersection;
  }

  const char* failed_constraint_name() const { return failed_constraint_name_; }
  const AudioDeviceSet& audio_device_set() const { return audio_device_set_; }
  const BoolSet& hotword_enabled_set() const { return hotword_enabled_set_; }
  const BoolSet& disable_local_echo_set() const {
    return disable_local_echo_set_;
  }
  const BoolSet& render_to_associated_sink_set() const {
    return render_to_associated_sink_set_;
  }
  const BoolSet& echo_cancellation_set() const {
    return echo_cancellation_set_;
  }
  const BoolSet& goog_echo_cancellation_set() const {
    return goog_echo_cancellation_set_;
  }
  const BoolSet& goog_da_echo_cancellation_set() const {
    return goog_da_echo_cancellation_set_;
  }
  const BoolSet& goog_audio_mirroring_set() const {
    return goog_audio_mirroring_set_;
  }
  const BoolSet& goog_auto_gain_control_set() const {
    return goog_auto_gain_control_set_;
  }
  const BoolSet& goog_experimental_echo_cancellation_set() const {
    return goog_experimental_echo_cancellation_set_;
  }
  const BoolSet& goog_typing_noise_detection_set() const {
    return goog_typing_noise_detection_set_;
  }
  const BoolSet& goog_noise_suppression_set() const {
    return goog_noise_suppression_set_;
  }
  const BoolSet& goog_experimental_noise_suppression_set() const {
    return goog_experimental_noise_suppression_set_;
  }
  const BoolSet& goog_beamforming_set() const { return goog_beamforming_set_; }
  const BoolSet& goog_highpass_filter_set() const {
    return goog_highpass_filter_set_;
  }
  const BoolSet& goog_experimental_auto_gain_control_set() const {
    return goog_experimental_auto_gain_control_set_;
  }
  const StringSet& goog_array_geometry_set() const {
    return goog_array_geometry_set_;
  }

 private:
  template <typename SetType, typename ConstraintType>
  void MaybeUpdateFailedConstraintName(
      SetType AudioDeviceCaptureCandidates::*set,
      const ConstraintType& constraint) {
    DCHECK(constraint.GetName());
    if ((this->*set).IsEmpty()) {
      failed_constraint_name_ = constraint.GetName();
    }
  }

  void MaybeUpdateFailedNonDeviceConstraintName() {
    blink::WebMediaTrackConstraintSet constraint_set;
    MaybeUpdateFailedConstraintName(
        &AudioDeviceCaptureCandidates::hotword_enabled_set_,
        constraint_set.hotword_enabled);
    MaybeUpdateFailedConstraintName(
        &AudioDeviceCaptureCandidates::disable_local_echo_set_,
        constraint_set.disable_local_echo);
    MaybeUpdateFailedConstraintName(
        &AudioDeviceCaptureCandidates::render_to_associated_sink_set_,
        constraint_set.render_to_associated_sink);
    MaybeUpdateFailedConstraintName(
        &AudioDeviceCaptureCandidates::echo_cancellation_set_,
        constraint_set.echo_cancellation);
    MaybeUpdateFailedConstraintName(
        &AudioDeviceCaptureCandidates::goog_echo_cancellation_set_,
        constraint_set.goog_echo_cancellation);
    MaybeUpdateFailedConstraintName(
        &AudioDeviceCaptureCandidates::goog_da_echo_cancellation_set_,
        constraint_set.goog_da_echo_cancellation);
    MaybeUpdateFailedConstraintName(
        &AudioDeviceCaptureCandidates::goog_audio_mirroring_set_,
        constraint_set.goog_audio_mirroring);
    MaybeUpdateFailedConstraintName(
        &AudioDeviceCaptureCandidates::goog_auto_gain_control_set_,
        constraint_set.goog_auto_gain_control);
    MaybeUpdateFailedConstraintName(
        &AudioDeviceCaptureCandidates::goog_experimental_echo_cancellation_set_,
        constraint_set.goog_experimental_echo_cancellation);
    MaybeUpdateFailedConstraintName(
        &AudioDeviceCaptureCandidates::goog_typing_noise_detection_set_,
        constraint_set.goog_typing_noise_detection);
    MaybeUpdateFailedConstraintName(
        &AudioDeviceCaptureCandidates::goog_noise_suppression_set_,
        constraint_set.goog_noise_suppression);
    MaybeUpdateFailedConstraintName(
        &AudioDeviceCaptureCandidates::goog_experimental_noise_suppression_set_,
        constraint_set.goog_experimental_noise_suppression);
    MaybeUpdateFailedConstraintName(
        &AudioDeviceCaptureCandidates::goog_beamforming_set_,
        constraint_set.goog_beamforming);
    MaybeUpdateFailedConstraintName(
        &AudioDeviceCaptureCandidates::goog_highpass_filter_set_,
        constraint_set.goog_highpass_filter);
    MaybeUpdateFailedConstraintName(
        &AudioDeviceCaptureCandidates::goog_experimental_auto_gain_control_set_,
        constraint_set.goog_experimental_auto_gain_control);
    MaybeUpdateFailedConstraintName(
        &AudioDeviceCaptureCandidates::goog_array_geometry_set_,
        constraint_set.goog_array_geometry);
    CheckContradictoryEchoCancellation();
  }

  void CheckContradictoryEchoCancellation() {
    BoolSet echo_cancellation_intersection =
        echo_cancellation_set_.Intersection(goog_echo_cancellation_set_);
    // echoCancellation and googEchoCancellation constraints should not
    // contradict each other. Mark the set as empty if they do.
    if (echo_cancellation_intersection.IsEmpty()) {
      failed_constraint_name_ =
          blink::WebMediaTrackConstraintSet().echo_cancellation.GetName();
    }
  }

  const char* failed_constraint_name_;

  // Device-related constraints.
  AudioDeviceSet audio_device_set_;

  // Constraints not related to audio processing.
  BoolSet hotword_enabled_set_;
  BoolSet disable_local_echo_set_;
  BoolSet render_to_associated_sink_set_;

  // Constraints that enable/disable audio processing.
  BoolSet echo_cancellation_set_;
  BoolSet goog_echo_cancellation_set_;
  BoolSet goog_da_echo_cancellation_set_;

  // Constraints that control audio-processing behavior.
  BoolSet goog_audio_mirroring_set_;
  BoolSet goog_auto_gain_control_set_;
  BoolSet goog_experimental_echo_cancellation_set_;
  BoolSet goog_typing_noise_detection_set_;
  BoolSet goog_noise_suppression_set_;
  BoolSet goog_experimental_noise_suppression_set_;
  BoolSet goog_beamforming_set_;
  BoolSet goog_highpass_filter_set_;
  BoolSet goog_experimental_auto_gain_control_set_;
  StringSet goog_array_geometry_set_;
};

// Fitness function for constraints involved in device selection.
//  Based on https://w3c.github.io/mediacapture-main/#dfn-fitness-distance
double DeviceInfoFitness(
    bool is_device_capture,
    const AudioDeviceInfo& device_info,
    const blink::WebMediaTrackConstraintSet& basic_constraint_set) {
  double fitness = 0.0;
  fitness += StringConstraintFitnessDistance(
      blink::WebString::FromASCII(device_info.device_id()),
      basic_constraint_set.device_id);

  if (!is_device_capture)
    return fitness;

  if (basic_constraint_set.sample_rate.HasIdeal()) {
    fitness += NumericConstraintFitnessDistance(
        device_info.SampleRate(), basic_constraint_set.sample_rate.Ideal());
  }

  if (basic_constraint_set.sample_size.HasIdeal()) {
    fitness += NumericConstraintFitnessDistance(
        device_info.SampleSize(), basic_constraint_set.sample_size.Ideal());
  }

  if (basic_constraint_set.channel_count.HasIdeal()) {
    fitness += NumericConstraintFitnessDistance(
        device_info.ChannelCount(), basic_constraint_set.channel_count.Ideal());
  }

  return fitness;
}

AudioDeviceInfo SelectDevice(
    const AudioDeviceSet& audio_device_set,
    const blink::WebMediaTrackConstraintSet& basic_constraint_set,
    const std::string& default_device_id,
    bool is_device_capture) {
  DCHECK(!audio_device_set.IsEmpty());
  if (audio_device_set.is_universal()) {
    DCHECK(!is_device_capture);
    std::string device_id;
    if (basic_constraint_set.device_id.HasIdeal()) {
      device_id = basic_constraint_set.device_id.Ideal().begin()->Utf8();
    }
    return AudioDeviceInfo(std::move(device_id));
  }

  std::vector<double> best_fitness({HUGE_VAL, HUGE_VAL});
  auto best_candidate = audio_device_set.elements().end();
  for (auto it = audio_device_set.elements().begin();
       it != audio_device_set.elements().end(); ++it) {
    std::vector<double> fitness;
    // First criterion is spec-based fitness function. Second criterion is
    // being the default device.
    fitness.push_back(
        DeviceInfoFitness(is_device_capture, *it, basic_constraint_set));
    fitness.push_back(it->device_id() == default_device_id ? 0.0 : HUGE_VAL);
    if (fitness < best_fitness) {
      best_fitness = std::move(fitness);
      best_candidate = it;
    }
  }
  DCHECK(best_candidate != audio_device_set.elements().end());
  return *best_candidate;
}

bool SelectBool(const BoolSet& set,
                const blink::BooleanConstraint& constraint,
                bool default_value) {
  DCHECK(!set.IsEmpty());
  if (constraint.HasIdeal() && set.Contains(constraint.Ideal())) {
    return constraint.Ideal();
  }

  // Return default value if unconstrained.
  if (set.is_universal()) {
    return default_value;
  }
  return set.FirstElement();
}

base::Optional<bool> SelectOptionalBool(
    const BoolSet& set,
    const blink::BooleanConstraint& constraint) {
  DCHECK(!set.IsEmpty());
  if (constraint.HasIdeal() && set.Contains(constraint.Ideal())) {
    return constraint.Ideal();
  }

  // Return no value if unconstrained.
  if (set.is_universal()) {
    return base::Optional<bool>();
  }
  return set.FirstElement();
}

base::Optional<std::string> SelectOptionalString(
    const StringSet& set,
    const blink::StringConstraint& constraint) {
  DCHECK(!set.IsEmpty());
  if (constraint.HasIdeal()) {
    for (const auto& ideal_candidate : constraint.Ideal()) {
      std::string candidate = ideal_candidate.Utf8();
      if (set.Contains(candidate)) {
        return candidate;
      }
    }
  }

  // Return no value if unconstrained.
  if (set.is_universal()) {
    return base::Optional<std::string>();
  }
  return set.FirstElement();
}

bool SelectEnableSwEchoCancellation(
    base::Optional<bool> echo_cancellation,
    base::Optional<bool> goog_echo_cancellation,
    const media::AudioParameters& audio_parameters,
    bool default_audio_processing_value) {
  // If there is hardware echo cancellation, return false.
  if (audio_parameters.IsValid() &&
      (audio_parameters.effects() & media::AudioParameters::ECHO_CANCELLER))
    return false;
  DCHECK(echo_cancellation && goog_echo_cancellation
             ? *echo_cancellation == *goog_echo_cancellation
             : true);
  if (echo_cancellation)
    return *echo_cancellation;
  if (goog_echo_cancellation)
    return *goog_echo_cancellation;

  return default_audio_processing_value;
}

const bool kDefaultGoogAudioMirroring = false;
#if defined(OS_ANDROID)
const bool kDefaultGoogExperimentalEchoCancellation = false;
#else
// Enable the extended filter mode AEC on all non-mobile platforms.
const bool kDefaultGoogExperimentalEchoCancellation = true;
#endif
const bool kDefaultGoogAutoGainControl = true;
const bool kDefaultGoogExperimentalAutoGainControl = true;
const bool kDefaultGoogNoiseSuppression = true;
const bool kDefaultGoogHighpassFilter = true;
const bool kDefaultGoogTypingNoiseDetection = true;
const bool kDefaultGoogExperimentalNoiseSuppression = true;
const bool kDefaultGoogBeamforming = true;

AudioProcessingProperties SelectAudioProcessingProperties(
    const AudioDeviceCaptureCandidates& candidates,
    const blink::WebMediaTrackConstraintSet& basic_constraint_set,
    const media::AudioParameters& audio_parameters,
    bool is_device_capture) {
  DCHECK(!candidates.IsEmpty());
  base::Optional<bool> echo_cancellation =
      SelectOptionalBool(candidates.echo_cancellation_set(),
                         basic_constraint_set.echo_cancellation);
  // Audio-processing properties are disabled by default for content capture, or
  // if the |echo_cancellation| constraint is false.
  bool default_audio_processing_value = true;
  if (!is_device_capture || (echo_cancellation && !*echo_cancellation))
    default_audio_processing_value = false;

  base::Optional<bool> goog_echo_cancellation =
      SelectOptionalBool(candidates.goog_echo_cancellation_set(),
                         basic_constraint_set.goog_echo_cancellation);

  AudioProcessingProperties properties;
  properties.enable_sw_echo_cancellation = SelectEnableSwEchoCancellation(
      echo_cancellation, goog_echo_cancellation, audio_parameters,
      default_audio_processing_value);
  properties.disable_hw_echo_cancellation =
      (echo_cancellation && !*echo_cancellation) ||
      (goog_echo_cancellation && !*goog_echo_cancellation);

  properties.goog_audio_mirroring = SelectBool(
      candidates.goog_audio_mirroring_set(),
      basic_constraint_set.goog_audio_mirroring, kDefaultGoogAudioMirroring);

  auto DefaultValue =
      [default_audio_processing_value](bool property_default) -> bool {
    return default_audio_processing_value && property_default;
  };

  properties.goog_auto_gain_control =
      SelectBool(candidates.goog_auto_gain_control_set(),
                 basic_constraint_set.goog_auto_gain_control,
                 DefaultValue(kDefaultGoogAutoGainControl));
  properties.goog_experimental_echo_cancellation =
      SelectBool(candidates.goog_experimental_echo_cancellation_set(),
                 basic_constraint_set.goog_experimental_echo_cancellation,
                 DefaultValue(kDefaultGoogExperimentalEchoCancellation));
  properties.goog_typing_noise_detection =
      SelectBool(candidates.goog_typing_noise_detection_set(),
                 basic_constraint_set.goog_typing_noise_detection,
                 DefaultValue(kDefaultGoogTypingNoiseDetection));
  properties.goog_noise_suppression =
      SelectBool(candidates.goog_noise_suppression_set(),
                 basic_constraint_set.goog_noise_suppression,
                 DefaultValue(kDefaultGoogNoiseSuppression));
  properties.goog_experimental_noise_suppression =
      SelectBool(candidates.goog_experimental_noise_suppression_set(),
                 basic_constraint_set.goog_experimental_noise_suppression,
                 DefaultValue(kDefaultGoogExperimentalNoiseSuppression));
  properties.goog_beamforming = SelectBool(
      candidates.goog_beamforming_set(), basic_constraint_set.goog_beamforming,
      DefaultValue(kDefaultGoogBeamforming));
  properties.goog_highpass_filter =
      SelectBool(candidates.goog_highpass_filter_set(),
                 basic_constraint_set.goog_highpass_filter,
                 DefaultValue(kDefaultGoogHighpassFilter));
  properties.goog_experimental_auto_gain_control =
      SelectBool(candidates.goog_experimental_auto_gain_control_set(),
                 basic_constraint_set.goog_experimental_auto_gain_control,
                 DefaultValue(kDefaultGoogExperimentalAutoGainControl));
  base::Optional<std::string> array_geometry =
      SelectOptionalString(candidates.goog_array_geometry_set(),
                           basic_constraint_set.goog_array_geometry);
  properties.goog_array_geometry =
      array_geometry && !array_geometry->empty()
          ? media::ParsePointsFromString(*array_geometry)
          : audio_parameters.mic_positions();

  return properties;
}

AudioCaptureSettings SelectResult(
    const AudioDeviceCaptureCandidates& candidates,
    const blink::WebMediaTrackConstraintSet& basic_constraint_set,
    const std::string& default_device_id,
    bool is_device_capture) {
  AudioDeviceInfo device_info =
      SelectDevice(candidates.audio_device_set(), basic_constraint_set,
                   default_device_id, is_device_capture);
  bool hotword_enabled =
      SelectBool(candidates.hotword_enabled_set(),
                 basic_constraint_set.hotword_enabled, false);
  bool disable_local_echo =
      SelectBool(candidates.disable_local_echo_set(),
                 basic_constraint_set.disable_local_echo, false);
  bool render_to_associated_sink =
      SelectBool(candidates.render_to_associated_sink_set(),
                 basic_constraint_set.render_to_associated_sink, false);

  AudioProcessingProperties audio_processing_properties =
      SelectAudioProcessingProperties(
          candidates, basic_constraint_set,
          // is_device_capture ? device_info.parameters() :
          // media::AudioParameters(),
          device_info.parameters(), is_device_capture);

  return AudioCaptureSettings(device_info.device_id(),
                              // is_device_capture ? device_info.parameters() :
                              // media::AudioParameters(),
                              device_info.parameters(), hotword_enabled,
                              disable_local_echo, render_to_associated_sink,
                              audio_processing_properties);
}

}  // namespace

AudioCaptureSettings SelectSettingsAudioCapture(
    const AudioDeviceCaptureCapabilities& capabilities,
    const blink::WebMediaConstraints& constraints) {
  bool is_device_capture = IsDeviceCapture(constraints);
  if (is_device_capture && capabilities.empty())
    return AudioCaptureSettings();

  AudioDeviceCaptureCandidates candidates(constraints.Basic(), capabilities,
                                          is_device_capture);
  if (candidates.IsEmpty()) {
    return AudioCaptureSettings(candidates.failed_constraint_name());
  }

  for (const auto& advanced_set : constraints.Advanced()) {
    AudioDeviceCaptureCandidates advanced_candidates(advanced_set, capabilities,
                                                     is_device_capture);
    AudioDeviceCaptureCandidates intersection =
        candidates.Intersection(advanced_candidates);
    if (!intersection.IsEmpty())
      candidates = std::move(intersection);
  }
  DCHECK(!candidates.IsEmpty());

  std::string default_device_id;
  if (!capabilities.empty())
    default_device_id = (*capabilities.begin())->device_id;

  return SelectResult(candidates, constraints.Basic(), default_device_id,
                      IsDeviceCapture(constraints));
}

}  // namespace content
